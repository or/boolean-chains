use rayon::prelude::*;
use regex::Regex;
use std::fs;
use std::io::{self, BufRead};
use walkdir::WalkDir;

#[derive(Default)]
struct Stats {
    solutions: Vec<String>,
    total_chains: u64,
    total_secs: f64,
}

fn process_file(
    path: &std::path::Path,
    re_x1: &Regex,
    re_chains: &Regex,
    re_real: &Regex,
) -> Stats {
    let mut stats = Stats::default();

    if let Ok(file) = fs::File::open(path) {
        let reader = io::BufReader::new(file);

        for line in reader.lines().flatten() {
            if re_x1.is_match(&line) {
                stats.solutions.push(line.clone());
            }

            if re_chains.is_match(&line) {
                if let Some(field) = line.split_whitespace().nth(2) {
                    if field != "18446744073709551615" {
                        if let Ok(val) = field.parse::<u64>() {
                            stats.total_chains += val;
                        }
                    }
                }
            }

            if re_real.is_match(&line) {
                if let Some(field) = line.split_whitespace().nth(1) {
                    if field.contains('m') {
                        let parts: Vec<_> = field.split('m').collect();
                        if parts.len() == 2 {
                            if let Ok(mins) = parts[0].parse::<f64>() {
                                let secs_str = parts[1].trim_end_matches('s');
                                if let Ok(secs) = secs_str.parse::<f64>() {
                                    stats.total_secs += mins * 60.0 + secs;
                                }
                            }
                        }
                    } else {
                        if let Ok(secs) = field.trim_end_matches('s').parse::<f64>() {
                            stats.total_secs += secs;
                        }
                    }
                }
            }
        }
    }

    stats
}

fn main() {
    let re_x1 = Regex::new(r"x1").unwrap();
    let re_chains = Regex::new(r"\bchains\b").unwrap();
    let re_real = Regex::new(r"\breal\b").unwrap();

    // Collect all matching files first
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

    // Process files in parallel
    let final_stats = files
        .par_iter()
        .map(|path| process_file(path, &re_x1, &re_chains, &re_real))
        .reduce(
            || Stats::default(),
            |mut a, b| {
                a.solutions.extend(b.solutions);
                a.total_chains += b.total_chains;
                a.total_secs += b.total_secs;
                a
            },
        );

    println!("solutions:");
    for s in final_stats.solutions {
        println!("{s}");
    }

    println!();
    println!("total number of chains: {}", final_stats.total_chains);

    let total_days = final_stats.total_secs / 3600.0 / 24.0;
    println!("total number of days:   {}", total_days.round() as u64);
    println!();

    if final_stats.total_secs > 0.0 {
        let speed = final_stats.total_chains as f64 / final_stats.total_secs;
        println!("speed: {:.2} chains/sec", speed);
    }
}
