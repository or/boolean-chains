use regex::Regex;
use std::fs;
use std::io::{self, BufRead};
use std::path::Path;
use walkdir::WalkDir;

fn main() -> io::Result<()> {
    // Regexes to match lines of interest
    let re_x1 = Regex::new(r"x1").unwrap();
    let re_chains = Regex::new(r"\bchains\b").unwrap();
    let re_real = Regex::new(r"\breal\b").unwrap();

    println!("solutions:");

    let mut total_chains: u64 = 0;
    let mut total_secs: f64 = 0.0;

    for entry in WalkDir::new(".")
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
    {
        let file = fs::File::open(entry.path())?;
        let reader = io::BufReader::new(file);

        for line in reader.lines() {
            let line = line?;

            if re_x1.is_match(&line) {
                println!("{}", line);
            }

            if re_chains.is_match(&line) {
                // the zsh version took the 3rd field → assume space separated
                if let Some(field) = line.split_whitespace().nth(2) {
                    if field != "18446744073709551615" {
                        if let Ok(val) = field.parse::<u64>() {
                            total_chains += val;
                        }
                    }
                }
            }

            if re_real.is_match(&line) {
                // 2nd field
                if let Some(field) = line.split_whitespace().nth(1) {
                    // Normalize the original sed replacements
                    let normalized = field
                        .replace("m", "*60+")
                        .replace("s", "")
                        .replace("..", "0.");

                    // Try to evaluate simple times like XmYs
                    // Example inputs: "12.34", "1m23.45s"
                    if field.contains('m') {
                        // basic "MmSs" style
                        let parts: Vec<_> = field.split('m').collect();
                        if parts.len() == 2 {
                            if let Ok(mins) = parts[0].parse::<f64>() {
                                let secs_str = parts[1].trim_end_matches('s');
                                if let Ok(secs) = secs_str.parse::<f64>() {
                                    total_secs += mins * 60.0 + secs;
                                }
                            }
                        }
                    } else {
                        if let Ok(secs) = normalized.parse::<f64>() {
                            total_secs += secs;
                        }
                    }
                }
            }
        }
    }

    println!();
    println!("total number of chains: {}", total_chains);

    let total_days = total_secs / 3600.0 / 24.0;
    println!("total number of days:   {}", total_days.round() as u64);
    println!();

    if total_secs > 0.0 {
        let speed = total_chains as f64 / total_secs;
        println!("speed: {:.2} chains/sec", speed);
    }

    Ok(())
}
