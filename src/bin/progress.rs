use rayon::prelude::*;
use std::collections::{HashMap, HashSet};
use std::fs;
use std::io::{self, BufRead, Write};
use std::path::{Path, PathBuf};
use std::sync::Mutex;
use walkdir::WalkDir;

mod jobs;

#[derive(Default, Debug, Clone)]
struct MatrixRow {
    n: u64,
    sum: u64,
    min: u64,
    max: u64,
}

impl MatrixRow {
    fn merge(&mut self, other: &MatrixRow) {
        if other.n == 0 {
            return;
        }
        self.n += other.n;
        self.sum += other.sum;
        self.min = self.min.min(other.min);
        self.max = self.max.max(other.max);
    }
}

#[derive(Default)]
struct Stats {
    total_chains: u64,
    total_secs: f64,
    jobs: HashSet<String>,
    matrix: HashMap<u32, MatrixRow>,
}

#[derive(Default)]
struct ChunkResults {
    results: HashMap<String, Vec<u64>>,
}

fn parse_time(field: &str) -> Option<f64> {
    if let Some(idx) = field.find('m') {
        let (mins, rest) = field.split_at(idx);
        let secs = rest.trim_start_matches('m').trim_end_matches('s');
        let mins: f64 = mins.parse().ok()?;
        let secs: f64 = if secs.is_empty() {
            0.0
        } else {
            secs.parse().ok()?
        };
        Some(mins * 60.0 + secs)
    } else {
        let s = field.trim_end_matches('s');
        s.parse::<f64>().ok()
    }
}

#[derive(Default)]
struct Chunk {
    args: Option<String>,
    chunk_id: Vec<u64>,
    total_chains: Option<u64>,
    secs: Option<f64>,
    in_matrix: bool,
    in_progress: bool,
    matrix: HashMap<u32, MatrixRow>,
    real_seen: bool,
    ignore: bool,
    corrupt: bool,
    last_tuple: Vec<u64>,
}

fn finalize_chunk_into_stats(chunk: &Chunk, stats: &mut Stats) {
    if chunk.ignore || chunk.corrupt {
        return;
    }
    if let (Some(chains), Some(secs)) = (chunk.total_chains, chunk.secs) {
        stats.total_chains += chains;
        stats.total_secs += secs;
        if let Some(ref args) = chunk.args {
            if !args.is_empty() {
                stats.jobs.insert(args.clone());
            }
        }

        for (k, row) in chunk.matrix.iter() {
            stats
                .matrix
                .entry(*k)
                .and_modify(|row_a| row_a.merge(row))
                .or_insert_with(|| row.clone());
        }
    }
}

fn finalize_chunk_into_results(chunk: &Chunk, results: &mut ChunkResults) {
    if chunk.ignore || chunk.corrupt {
        return;
    }
    if let (Some(chains), Some(_secs)) = (chunk.total_chains, chunk.secs) {
        if let Some(ref args) = chunk.args {
            if !args.is_empty() {
                results
                    .results
                    .entry(args.clone())
                    .or_insert_with(Vec::new)
                    .push(chains);
            }
        }
    }
}

fn process_file(path: &Path) -> (Stats, ChunkResults, bool) {
    let mut stats = Stats::default();
    let mut results = ChunkResults::default();
    let mut current = Chunk::default();
    let mut file_has_corrupt = false;

    if let Ok(file) = fs::File::open(path) {
        let reader = io::BufReader::new(file);

        for line in reader.lines().flatten() {
            let line_trim = line.trim_end();

            if line_trim.starts_with("x1 = ") {
                if let Some(fname) = path.file_name().and_then(|n| n.to_str()) {
                    println!("{}:\n{}", fname, line_trim);
                }
            } else if line_trim.contains("Running command:") {
                if current.corrupt {
                    file_has_corrupt = true;
                }
                if current.total_chains.is_some()
                    && current.total_chains.unwrap() > 0
                    && current.real_seen
                    && !current.ignore
                    && !current.corrupt
                {
                    finalize_chunk_into_stats(&current, &mut stats);
                    finalize_chunk_into_results(&current, &mut results);
                }
                current = Chunk::default();

                let rest = line_trim
                    .split_once("Running command:")
                    .map(|(_, after)| after.trim())
                    .unwrap_or("");
                if !rest.is_empty() {
                    let mut tokens = rest.split_whitespace();
                    let _exe = tokens.next();
                    let args_vec: Vec<&str> = tokens.collect();
                    if !args_vec.is_empty() {
                        current.args = Some(args_vec.join(" "));
                        // Parse chunk ID
                        current.chunk_id = args_vec
                            .iter()
                            .filter_map(|s| s.parse::<u64>().ok())
                            .collect();
                    } else {
                        current.args = Some(String::new());
                    }
                }
            } else if line_trim.starts_with("total chains") {
                current.in_progress = false;
                if let Some(tok) = line_trim.split_whitespace().last() {
                    if tok == "18446744073709551615" {
                        current.ignore = true;
                    } else if let Ok(val) = tok.parse::<u64>() {
                        current.total_chains = Some(val);
                    }
                }
            } else if line_trim.starts_with("real") {
                if let Some(field) = line_trim.split_whitespace().nth(1) {
                    if let Some(secs) = parse_time(field) {
                        current.secs = Some(secs);
                        current.real_seen = true;
                    }
                }
                if current.corrupt {
                    file_has_corrupt = true;
                }
                if current.real_seen
                    && current.total_chains.is_some()
                    && current.total_chains.unwrap() > 0
                    && !current.ignore
                    && !current.corrupt
                {
                    finalize_chunk_into_stats(&current, &mut stats);
                    finalize_chunk_into_results(&current, &mut results);
                }
                current = Chunk::default();
            } else if line_trim.starts_with("new expressions at chain length:") {
                current.in_progress = false;
                current.in_matrix = true;
            } else if current.in_matrix {
                if line_trim.is_empty() || line_trim.starts_with("---") {
                    current.in_matrix = false;
                    continue;
                }

                if let Some(colon_idx) = line_trim.find(':') {
                    let n_id_str = line_trim[..colon_idx].trim();
                    if let Ok(n_id) = n_id_str.parse::<u32>() {
                        let rest = line_trim[colon_idx + 1..].trim();
                        let cols: Vec<&str> = rest.split_whitespace().collect();
                        if cols.len() == 5 {
                            if let (Ok(n), Ok(sum), Ok(_avg), Ok(min), Ok(max)) = (
                                cols[0].parse::<u64>(),
                                cols[1].parse::<u64>(),
                                cols[2].parse::<f64>(),
                                cols[3].parse::<u64>(),
                                cols[4].parse::<u64>(),
                            ) {
                                let row = MatrixRow { n, sum, min, max };
                                current.matrix.insert(n_id, row);
                            }
                        }
                    }
                }
            } else if current.in_progress {
                // Validate progress lines
                if let Some(space_idx) = line_trim.rfind(' ') {
                    let tuple_part = &line_trim[..space_idx];
                    let numbers: Vec<u64> = tuple_part
                        .split(',')
                        .map(|s| s.trim())
                        .filter_map(|s| s.parse::<u64>().ok())
                        .collect();

                    if numbers.len() >= current.chunk_id.len() + 1 {
                        // Check if it starts with chunk ID
                        let prefix_matches =
                            numbers[..current.chunk_id.len()] == current.chunk_id[..];

                        if prefix_matches {
                            // Check if tuple is strictly increasing
                            let mut valid_tuple = true;
                            for i in 1..numbers.len() {
                                if numbers[i] <= numbers[i - 1] {
                                    valid_tuple = false;
                                    break;
                                }
                            }

                            if valid_tuple {
                                // Check if this tuple is greater than the last one
                                if !current.last_tuple.is_empty() {
                                    if numbers <= current.last_tuple {
                                        current.corrupt = true;
                                    }
                                }
                                current.last_tuple = numbers;
                            } else {
                                current.corrupt = true;
                            }
                        } else {
                            current.corrupt = true;
                        }
                    }
                }
            } else if line_trim.starts_with("---") {
                // Entering progress section after first ---
                if current.args.is_some() && !current.in_progress {
                    current.in_progress = true;
                    current.last_tuple.clear();
                }
            }
        }

        if current.corrupt {
            file_has_corrupt = true;
        }
    }

    (stats, results, file_has_corrupt)
}

fn main() {
    let files: Vec<_> = WalkDir::new(".")
        .into_iter()
        .filter_map(Result::ok)
        .filter(|e| {
            if !e.file_type().is_file() {
                return false;
            }

            let file_name = match e.path().file_name().and_then(|n| n.to_str()) {
                Some(s) => s,
                None => return false,
            };

            if !file_name.ends_with("output") {
                return false;
            }

            if let Some(base) = file_name.strip_suffix("__file_output") {
                if jobs::JOBS_TO_IGNORE.contains(base) {
                    return false;
                }
            }

            true
        })
        .map(|e| e.into_path())
        .collect();

    let all_results = Mutex::new(ChunkResults::default());
    let corrupt_files = Mutex::new(HashSet::<PathBuf>::new());

    let final_stats = files
        .par_iter()
        .map(|path| {
            let (stats, results, has_corrupt) = process_file(path);

            if has_corrupt {
                corrupt_files.lock().unwrap().insert(path.clone());
            }

            let mut all_res = all_results.lock().unwrap();
            for (chunk_id, chains) in results.results {
                all_res
                    .results
                    .entry(chunk_id)
                    .or_insert_with(Vec::new)
                    .extend(chains);
            }

            stats
        })
        .reduce(
            || Stats::default(),
            |mut a, b| {
                a.total_chains += b.total_chains;
                a.total_secs += b.total_secs;
                a.jobs.extend(b.jobs);

                for (k, row_b) in b.matrix.iter() {
                    a.matrix
                        .entry(*k)
                        .and_modify(|row_a| row_a.merge(row_b))
                        .or_insert_with(|| row_b.clone());
                }

                a
            },
        );

    println!();
    println!("total number of chains: {}", final_stats.total_chains);
    println!("total number of jobs:   {}", final_stats.jobs.len());

    let total_days = final_stats.total_secs / 3600.0 / 24.0;
    let total_years = total_days / 365.2524;
    println!(
        "total duration:         {} days / {:.2} years",
        total_days.round() as u64,
        total_years
    );

    println!();

    if final_stats.total_secs > 0.0 {
        let speed = final_stats.total_chains as f64 / final_stats.total_secs;
        println!("speed: {:.2} chains/sec", speed);
    }

    println!("\nAggregated matrix:\n");
    println!(
        "{:>4}  {:>17} {:>17} {:>12} {:>12} {:>12}",
        "n", "count", "sum", "avg", "min", "max"
    );
    let mut keys: Vec<_> = final_stats.matrix.keys().cloned().collect();
    keys.sort();
    for k in keys {
        let row = &final_stats.matrix[&k];
        println!(
            "{:>4}: {:>17} {:>17} {:>12.2} {:>12} {:>12}",
            k,
            row.n,
            row.sum,
            if row.n == 0 {
                0.0
            } else {
                row.sum as f64 / row.n as f64
            },
            row.min,
            row.max
        );
    }

    let mut jobs: Vec<_> = final_stats.jobs.into_iter().collect();
    jobs.sort();
    let out_path = Path::new(".").join("processed-chunks.txt");
    let content = if jobs.is_empty() {
        String::new()
    } else {
        jobs.join("\n") + "\n"
    };
    if let Err(e) = fs::write(&out_path, content) {
        eprintln!("Failed to write {}: {}", out_path.display(), e);
    } else {
        println!("\nWrote processed-chunks to {}", out_path.display());
    }

    let all_res = all_results.lock().unwrap();

    let jobs_count_path = Path::new(".").join("number-of-jobs-per-chunk.txt");
    match fs::File::create(&jobs_count_path) {
        Ok(mut file) => {
            for (chunk_id, results) in all_res.results.iter() {
                if let Err(e) = writeln!(file, "{},{}", chunk_id, results.len()) {
                    eprintln!("Failed to write line: {}", e);
                }
            }
            println!(
                "Wrote number-of-jobs-per-chunk to {}",
                jobs_count_path.display()
            );
        }
        Err(e) => eprintln!("Failed to create {}: {}", jobs_count_path.display(), e),
    }

    let verified_path = Path::new(".").join("verified-chunks.txt");
    match fs::File::create(&verified_path) {
        Ok(mut file) => {
            for (chunk_id, results) in all_res.results.iter() {
                if results.len() >= 2 {
                    let unique: HashSet<_> = results.iter().collect();
                    if unique.len() < results.len() {
                        if let Err(e) = writeln!(file, "{}", chunk_id) {
                            eprintln!("Failed to write line: {}", e);
                        }
                    }
                }
            }
            println!("Wrote verified-chunks to {}", verified_path.display());
        }
        Err(e) => eprintln!("Failed to create {}: {}", verified_path.display(), e),
    }

    let mismatching_path = Path::new(".").join("mismatching-chunks.txt");
    match fs::File::create(&mismatching_path) {
        Ok(mut file) => {
            for (chunk_id, results) in all_res.results.iter() {
                if results.len() >= 2 {
                    let unique: HashSet<_> = results.iter().collect();
                    if unique.len() == results.len() {
                        if let Err(e) = writeln!(file, "{}", chunk_id) {
                            eprintln!("Failed to write line: {}", e);
                        }
                    }
                }
            }
            println!("Wrote mismatching-chunks to {}", mismatching_path.display());
        }
        Err(e) => eprintln!("Failed to create {}: {}", mismatching_path.display(), e),
    }

    let corrupt_path = Path::new(".").join("corrupt-output-files.txt");
    match fs::File::create(&corrupt_path) {
        Ok(mut file) => {
            let corrupt = corrupt_files.lock().unwrap();
            for path in corrupt.iter() {
                if let Err(e) = writeln!(file, "{}", path.display()) {
                    eprintln!("Failed to write line: {}", e);
                }
            }
            println!(
                "Wrote corrupt-output-files to {} ({} files)",
                corrupt_path.display(),
                corrupt.len()
            );
        }
        Err(e) => eprintln!("Failed to create {}: {}", corrupt_path.display(), e),
    }
}
