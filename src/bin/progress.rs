use rayon::prelude::*;
use std::collections::HashSet;
use std::fs;
use std::io::{self, BufRead};
use std::path::Path;
use walkdir::WalkDir;

#[derive(Default)]
struct Stats {
    total_chains: u64,
    total_secs: f64,
    jobs: HashSet<String>,
}

/// Parse a "real" time field like "12.34", "1m23s", "10s"
fn parse_time(field: &str) -> Option<f64> {
    if let Some(idx) = field.find('m') {
        // Format "MmSs"
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
        // Format "Ss.s" or "S"
        let s = field.trim_end_matches('s');
        s.parse::<f64>().ok()
    }
}

fn process_file(path: &Path) -> Stats {
    let mut stats = Stats::default();

    if let Ok(file) = fs::File::open(path) {
        let reader = io::BufReader::new(file);

        for line in reader.lines().flatten() {
            if line.starts_with("x1 = ") {
                if let Some(fname) = path.file_name().and_then(|n| n.to_str()) {
                    println!("{}:\n{}", fname, line);
                }
            }

            if line.starts_with("total chains") {
                if let Some(field) = line.split_whitespace().nth(2) {
                    if field != "18446744073709551615" {
                        if let Ok(val) = field.parse::<u64>() {
                            stats.total_chains += val;
                        }
                    }
                }
            }

            if line.starts_with("real") {
                if let Some(field) = line.split_whitespace().nth(1) {
                    if let Some(secs) = parse_time(field) {
                        stats.total_secs += secs;
                    }
                }
            }

            if line.starts_with("Running command:") {
                let mut parts = line.splitn(4, ' ');
                parts.next(); // "Running"
                parts.next(); // "command:"
                parts.next(); // executable
                if let Some(args) = parts.next() {
                    if !args.is_empty() {
                        stats.jobs.insert(args.to_string());
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
            a
        },
    );

    println!();
    println!("total number of chains: {}", final_stats.total_chains);

    let total_days = final_stats.total_secs / 3600.0 / 24.0;
    println!("total number of days:   {}", total_days.round() as u64);

    println!("total number of jobs:   {}", final_stats.jobs.len());

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
}
