use rayon::prelude::*;
use std::collections::{HashMap, HashSet};
use std::fs;
use std::io::{self, BufRead};
use std::path::Path;
use walkdir::WalkDir;

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

/// Parse a "real" time field like "12.34", "1m23s", "10s"
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

fn process_file(path: &Path) -> Stats {
    let mut stats = Stats::default();
    let mut in_matrix = false;

    if let Ok(file) = fs::File::open(path) {
        let reader = io::BufReader::new(file);

        for line in reader.lines().flatten() {
            if line.starts_with("x1 = ") {
                if let Some(fname) = path.file_name().and_then(|n| n.to_str()) {
                    println!("{}:\n{}", fname, line);
                }
            } else if line.starts_with("total chains") {
                if let Some(field) = line.split_whitespace().nth(2) {
                    if field != "18446744073709551615" {
                        if let Ok(val) = field.parse::<u64>() {
                            stats.total_chains += val;
                        }
                    }
                }
            } else if line.starts_with("real") {
                in_matrix = false;
                if let Some(field) = line.split_whitespace().nth(1) {
                    if let Some(secs) = parse_time(field) {
                        stats.total_secs += secs;
                    }
                }
            } else if line.starts_with("Running command:") {
                let mut parts = line.splitn(4, ' ');
                parts.next(); // "Running"
                parts.next(); // "command:"
                parts.next(); // executable
                if let Some(args) = parts.next() {
                    if !args.is_empty() {
                        stats.jobs.insert(args.to_string());
                    }
                }
            } else if line.starts_with("new expressions at chain length:") {
                in_matrix = true;
                continue;
            } else if in_matrix {
                // Matrix ends when we hit an empty line or "real"/"sys"
                if line.is_empty() || line.starts_with("---") {
                    in_matrix = false;
                    continue;
                }

                // Expected row format like:
                // " 4:                0                        15                0               14               15"
                let parts: Vec<_> = line
                    .split_whitespace()
                    .filter(|s| !s.ends_with(':'))
                    .collect();
                if parts.len() == 5 {
                    if let (Ok(n_id), Ok(n), Ok(sum), Ok(_avg), Ok(min), Ok(max)) = (
                        line.split(':').next().unwrap_or("").trim().parse::<u32>(),
                        parts[0].parse::<u64>(),
                        parts[1].parse::<u64>(),
                        parts[2].parse::<f64>(),
                        parts[3].parse::<u64>(),
                        parts[4].parse::<u64>(),
                    ) {
                        stats.matrix.insert(n_id, MatrixRow { n, sum, min, max });
                    }
                }
            }
        }
    }

    stats
}

fn main() {
    // Collect all *output files
    let files: Vec<_> = WalkDir::new(".")
        .into_iter()
        .filter_map(Result::ok)
        .filter(|e| {
            e.file_type().is_file()
                && e.path()
                    .file_name()
                    .and_then(|n| n.to_str())
                    .map(|s| s.ends_with("output"))
                    .unwrap_or(false)
        })
        .map(|e| e.into_path())
        .collect();

    let final_stats = files.par_iter().map(|path| process_file(path)).reduce(
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

    // // Print sorted job args list
    // let mut jobs: Vec<_> = final_stats.jobs.into_iter().collect();
    // jobs.sort();
    // for job in jobs {
    //     println!("job args: {}", job);
    // }

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
}
