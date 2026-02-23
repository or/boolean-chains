use indicatif::{ProgressBar, ProgressStyle};
use rayon::prelude::*;
use rusqlite::{params, Connection};
use std::collections::{HashMap, HashSet};
use std::fs;
use std::io::{self, BufRead};
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
    matrix: HashMap<u32, MatrixRow>,
}

struct ParsedChunk {
    chunk_id: String,
    total_chains: u64,
    secs: f64,
    corrupt: bool,
    matrix: HashMap<u32, MatrixRow>,
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
    real_secs: Option<f64>,
    user_secs: Option<f64>,
    in_matrix: bool,
    in_progress: bool,
    matrix: HashMap<u32, MatrixRow>,
    real_seen: bool,
    ignore: bool,
    corrupt: bool,
    last_tuple: Vec<u64>,
}

fn finalize_chunk(chunk: &Chunk, stats: &mut Stats, parsed: &mut Vec<ParsedChunk>) {
    if chunk.ignore {
        return;
    }
    let secs = match (chunk.real_secs, chunk.user_secs) {
        (Some(r), Some(u)) => Some(r.max(u)),
        (Some(r), None) => Some(r),
        (None, Some(u)) => Some(u),
        (None, None) => None,
    };
    if let (Some(chains), Some(secs)) = (chunk.total_chains, secs) {
        let chunk_id = chunk.args.as_deref().unwrap_or("").to_string();

        if !chunk.corrupt {
            stats.total_chains += chains;
            stats.total_secs += secs;
            for (k, row) in chunk.matrix.iter() {
                stats
                    .matrix
                    .entry(*k)
                    .and_modify(|row_a| row_a.merge(row))
                    .or_insert_with(|| row.clone());
            }
        }

        if !chunk_id.is_empty() {
            parsed.push(ParsedChunk {
                chunk_id,
                total_chains: chains,
                secs,
                corrupt: chunk.corrupt,
                matrix: chunk.matrix.clone(),
            });
        }
    }
}

fn process_file(path: &Path) -> (Stats, Vec<ParsedChunk>, bool) {
    let mut stats = Stats::default();
    let mut parsed: Vec<ParsedChunk> = Vec::new();
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
                {
                    finalize_chunk(&current, &mut stats, &mut parsed);
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
            } else if line_trim.starts_with("user") {
                if let Some(field) = line_trim.split_whitespace().nth(1) {
                    current.user_secs = parse_time(field);
                }
            } else if line_trim.starts_with("real") {
                if let Some(field) = line_trim.split_whitespace().nth(1) {
                    if let Some(real) = parse_time(field) {
                        current.real_secs = Some(real);
                        current.real_seen = true;
                    }
                }
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
                if let Some(space_idx) = line_trim.rfind(' ') {
                    let tuple_part = &line_trim[..space_idx];
                    let numbers: Vec<u64> = tuple_part
                        .split(',')
                        .map(|s| s.trim())
                        .filter_map(|s| s.parse::<u64>().ok())
                        .collect();

                    if numbers.len() >= current.chunk_id.len() + 1 {
                        let prefix_matches =
                            numbers[..current.chunk_id.len()] == current.chunk_id[..];

                        if prefix_matches {
                            let mut valid_tuple = true;
                            for i in 1..numbers.len() {
                                if numbers[i] <= numbers[i - 1] {
                                    valid_tuple = false;
                                    break;
                                }
                            }

                            if valid_tuple {
                                if !current.last_tuple.is_empty() && numbers <= current.last_tuple {
                                    current.corrupt = true;
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
                if current.args.is_some() && !current.in_progress {
                    if current.real_seen {
                        // Closing separator — all time lines parsed, finalize.
                        if current.corrupt {
                            file_has_corrupt = true;
                        }
                        if current.total_chains.is_some() && current.total_chains.unwrap() > 0 {
                            finalize_chunk(&current, &mut stats, &mut parsed);
                        }
                        current = Chunk::default();
                    } else {
                        // Opening separator — enter progress section.
                        current.in_progress = true;
                        current.last_tuple.clear();
                    }
                }
            }
        }

        if current.corrupt {
            file_has_corrupt = true;
        }
    }

    (stats, parsed, file_has_corrupt)
}

fn open_db(path: &str) -> Connection {
    let conn = Connection::open(path).unwrap_or_else(|e| {
        eprintln!("Failed to open database {}: {}", path, e);
        std::process::exit(1);
    });
    conn.execute_batch(
        "
        PRAGMA journal_mode=WAL;
        PRAGMA synchronous=NORMAL;

        CREATE TABLE IF NOT EXISTS files (
            id      INTEGER PRIMARY KEY,
            path    TEXT NOT NULL UNIQUE,
            corrupt INTEGER NOT NULL DEFAULT 0,
            ignore  INTEGER NOT NULL DEFAULT 0
        );

        CREATE INDEX IF NOT EXISTS files_ignore_corrupt_idx ON files(ignore, corrupt);

        CREATE TABLE IF NOT EXISTS chunks (
            id           INTEGER PRIMARY KEY,
            file_id      INTEGER NOT NULL REFERENCES files(id),
            chunk_id     TEXT NOT NULL,
            total_chains INTEGER NOT NULL,
            secs         REAL NOT NULL,
            bad          INTEGER NOT NULL DEFAULT 0,
            corrupt      INTEGER NOT NULL DEFAULT 0,
            ignore       INTEGER NOT NULL DEFAULT 0,
            verified     INTEGER NOT NULL DEFAULT 0,
            wrong        INTEGER NOT NULL DEFAULT 0
        );

        CREATE INDEX IF NOT EXISTS chunks_chunk_id_idx ON chunks(chunk_id);
        CREATE INDEX IF NOT EXISTS chunks_covering_idx ON chunks(ignore, corrupt, chunk_id, total_chains);
        CREATE INDEX IF NOT EXISTS chunks_chunk_id_verified_idx ON chunks(chunk_id, verified, wrong);
        CREATE INDEX IF NOT EXISTS chunks_file_id_verified_idx ON chunks(file_id, verified, wrong);
        CREATE INDEX IF NOT EXISTS chunks_wrong_idx ON chunks(wrong);

        CREATE TABLE IF NOT EXISTS chunk_matrix (
            id           INTEGER PRIMARY KEY,
            chunk_row_id INTEGER NOT NULL REFERENCES chunks(id),
            chain_length INTEGER NOT NULL,
            n            INTEGER NOT NULL,
            sum          INTEGER NOT NULL,
            min          INTEGER NOT NULL,
            max          INTEGER NOT NULL,
            UNIQUE(chunk_row_id, chain_length)
        );
    ",
    )
    .unwrap_or_else(|e| {
        eprintln!("Failed to create schema: {}", e);
        std::process::exit(1);
    });
    conn
}

fn write_batch_to_db(conn: &Connection, batch: &[(String, Vec<ParsedChunk>, bool, bool)]) {
    let mut file_stmt = conn
        .prepare_cached("INSERT OR IGNORE INTO files (path, corrupt, ignore) VALUES (?1, ?2, ?3)")
        .expect("Failed to prepare file insert");

    let mut file_id_stmt = conn
        .prepare_cached("SELECT id FROM files WHERE path = ?1")
        .expect("Failed to prepare file id query");

    let mut chunk_stmt = conn
        .prepare_cached(
            "INSERT INTO chunks (file_id, chunk_id, total_chains, secs, corrupt, ignore)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
        )
        .expect("Failed to prepare chunk insert");

    let mut matrix_stmt = conn
        .prepare_cached(
            "INSERT OR IGNORE INTO chunk_matrix
             (chunk_row_id, chain_length, n, sum, min, max)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
        )
        .expect("Failed to prepare chunk_matrix insert");

    for (path_str, parsed, file_corrupt, file_ignore) in batch {
        file_stmt
            .execute(params![path_str, *file_corrupt as i64, *file_ignore as i64])
            .unwrap_or_else(|e| {
                eprintln!("Failed to insert file {}: {}", path_str, e);
                0
            });

        let file_id: i64 = file_id_stmt
            .query_row(params![path_str], |r| r.get(0))
            .unwrap_or_else(|e| {
                eprintln!("Failed to fetch file id for {}: {}", path_str, e);
                std::process::exit(1);
            });

        for chunk in parsed {
            chunk_stmt
                .execute(params![
                    file_id,
                    chunk.chunk_id,
                    chunk.total_chains as i64,
                    chunk.secs,
                    chunk.corrupt as i64,
                    *file_ignore as i64,
                ])
                .unwrap_or_else(|e| {
                    eprintln!("Failed to insert chunk {}: {}", chunk.chunk_id, e);
                    0
                });

            let chunk_row_id = conn.last_insert_rowid();

            for (chain_length, row) in &chunk.matrix {
                if row.sum == 0 {
                    continue;
                }
                matrix_stmt
                    .execute(params![
                        chunk_row_id,
                        *chain_length as i64,
                        row.n as i64,
                        row.sum as i64,
                        row.min as i64,
                        row.max as i64,
                    ])
                    .unwrap_or_else(|e| {
                        eprintln!(
                            "Failed to insert chunk_matrix row for chunk {}: {}",
                            chunk.chunk_id, e
                        );
                        0
                    });
            }
        }
    }
}

fn main() {
    let mut args = std::env::args().skip(1);
    let db_path = if let Some(flag) = args.next() {
        if flag == "--db" {
            args.next().unwrap_or_else(|| {
                eprintln!("--db requires a path argument");
                std::process::exit(1);
            })
        } else {
            eprintln!("Unknown argument: {}", flag);
            std::process::exit(1);
        }
    } else {
        "results.sqlite3".to_string()
    };

    let conn = open_db(&db_path);

    // Resolve the DB directory and current working directory once, so all
    // stored paths are relative to the DB file's location.
    let db_dir = Path::new(&db_path)
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or(Path::new("."))
        .canonicalize()
        .unwrap_or_else(|e| {
            eprintln!("Failed to resolve db directory: {}", e);
            std::process::exit(1);
        });
    let cwd = Path::new(".").canonicalize().unwrap_or_else(|e| {
        eprintln!("Failed to resolve current directory: {}", e);
        std::process::exit(1);
    });

    // Fetch all already-processed file paths in one query.
    let seen: HashSet<String> = {
        let mut stmt = conn
            .prepare("SELECT path FROM files")
            .expect("Failed to prepare seen-files query");
        stmt.query_map([], |r| r.get(0))
            .expect("Failed to query seen files")
            .filter_map(Result::ok)
            .collect()
    };

    let conn = Mutex::new(conn);

    // Each entry is (open_path, db_path):
    // - open_path: CWD-relative, used to open the file
    // - db_path:   DB-dir-relative, used for storage and dedup
    let files: Vec<(PathBuf, String)> = WalkDir::new(".")
        .into_iter()
        .filter_map(Result::ok)
        .filter_map(|e| {
            if !e.file_type().is_file() {
                return None;
            }

            let file_name = e.path().file_name().and_then(|n| n.to_str())?;

            if !file_name.ends_with("output") {
                return None;
            }

            let rel_to_cwd = match e.path().strip_prefix(".") {
                Ok(r) => r.to_path_buf(),
                Err(_) => e.path().to_path_buf(),
            };
            let abs = cwd.join(&rel_to_cwd);
            let db_rel = match abs.strip_prefix(&db_dir) {
                Ok(r) => format!("./{}", r.display()),
                Err(_) => {
                    eprintln!(
                        "Path {} is not relative to db directory {}",
                        abs.display(),
                        db_dir.display()
                    );
                    std::process::exit(1);
                }
            };

            if seen.contains(&db_rel) {
                return None;
            }

            Some((e.path().to_path_buf(), db_rel))
        })
        .collect();

    println!("Files to parse: {}", files.len());

    let pb = ProgressBar::new(files.len() as u64);
    pb.set_style(
        ProgressStyle::with_template(
            "[{elapsed_precise}] {bar:40.cyan/blue} {pos}/{len} files ({eta})",
        )
        .unwrap(),
    );
    pb.tick();

    const BATCH_SIZE: usize = 1000;

    let final_stats = files
        .par_chunks(BATCH_SIZE)
        .map(|batch| {
            // Parse all files in this batch sequentially — no lock held during parsing.
            let mut batch_stats = Stats::default();
            let mut results: Vec<(String, Vec<ParsedChunk>, bool, bool)> =
                Vec::with_capacity(batch.len());

            for (open_path, db_path) in batch {
                let (stats, parsed, file_corrupt) = process_file(open_path);

                let file_ignore = open_path
                    .file_name()
                    .and_then(|n| n.to_str())
                    .and_then(|n| n.strip_suffix("__file_output"))
                    .map(|base| jobs::JOBS_TO_IGNORE.contains(base))
                    .unwrap_or(false);

                batch_stats.total_chains += stats.total_chains;
                batch_stats.total_secs += stats.total_secs;
                for (k, row_b) in stats.matrix.iter() {
                    batch_stats
                        .matrix
                        .entry(*k)
                        .and_modify(|row_a| row_a.merge(row_b))
                        .or_insert_with(|| row_b.clone());
                }

                results.push((db_path.clone(), parsed, file_corrupt, file_ignore));
            }

            // Write the entire batch in a single transaction.
            {
                let conn = conn.lock().unwrap();
                conn.execute_batch("BEGIN")
                    .expect("Failed to begin transaction");
                write_batch_to_db(&conn, &results);
                conn.execute_batch("COMMIT")
                    .expect("Failed to commit transaction");
            }

            pb.inc(batch.len() as u64);
            batch_stats
        })
        .reduce(
            || Stats::default(),
            |mut a, b| {
                a.total_chains += b.total_chains;
                a.total_secs += b.total_secs;

                for (k, row_b) in b.matrix.iter() {
                    a.matrix
                        .entry(*k)
                        .and_modify(|row_a| row_a.merge(row_b))
                        .or_insert_with(|| row_b.clone());
                }

                a
            },
        );

    pb.finish_with_message("done");

    if final_stats.total_chains > 0 {
        println!();
        println!("total number of chains: {}", final_stats.total_chains);

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
    }
}
