use phf::{phf_set, Set};
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

static JOBS_TO_IGNORE: Set<&'static str> = phf_set! {
    // Mark6stark
    // https://boinc.berkeley.edu/central/show_host_detail.php?hostid=43350
    "batch_1009__job_job-000453",
    "batch_1054__job_job-000326",
    "batch_1055__job_job-000074",
    "batch_1057__job_job-000879",
    "batch_1068__job_job-000144",
    "batch_1072__job_job-000026",
    "batch_1072__job_job-000531",
    "batch_1080__job_job-000709",
    "batch_1085__job_job-000611",
    "batch_1086__job_job-000948",
    "batch_1093__job_job-000159",
    "batch_1094__job_job-000342",
    "batch_1095__job_job-000674",
    "batch_1097__job_job-000427",
    "batch_1098__job_job-000106",
    "batch_1098__job_job-000446",
    "batch_1098__job_job-000760",
    "batch_1100__job_job-000137",
    "batch_1100__job_job-000321",
    "batch_1100__job_job-000837",
    "batch_1101__job_job-000032",
    "batch_1101__job_job-000323",
    "batch_1101__job_job-000395",
    "batch_1101__job_job-000423",
    "batch_1102__job_job-000059",
    "batch_1103__job_job-000349",
    "batch_1103__job_job-000493",
    "batch_1103__job_job-000812",
    "batch_1104__job_job-000502",
    "batch_1104__job_job-000869",
    "batch_1104__job_job-000913",
    "batch_1105__job_job-000446",
    "batch_1105__job_job-000944",
    "batch_1106__job_job-000650",
    "batch_1106__job_job-000875",
    "batch_1106__job_job-000921",
    "batch_1107__job_job-000510",
    "batch_1108__job_job-000001",
    "batch_1108__job_job-000024",
    "batch_1108__job_job-000958",
    "batch_1109__job_job-000122",
    "batch_1110__job_job-000354",
    "batch_1110__job_job-000424",
    "batch_1111__job_job-000064",
    "batch_1111__job_job-000161",
    "batch_1112__job_job-000888",
    "batch_1113__job_job-000445",
    "batch_1113__job_job-000619",
    "batch_1114__job_job-000205",
    "batch_1114__job_job-000409",
    "batch_1114__job_job-000796",
    "batch_1114__job_job-000818",
    "batch_1114__job_job-000836",
    "batch_1117__job_job-000557",
    "batch_1117__job_job-000563",
    "batch_1118__job_job-000260",
    "batch_1119__job_job-000606",
    "batch_1120__job_job-000510",
    "batch_1123__job_job-000194",
    "batch_1123__job_job-000577",
    "batch_1123__job_job-000994",
    "batch_1124__job_job-000001",
    "batch_1124__job_job-000898",
    "batch_1126__job_job-000427",
    "batch_1126__job_job-000449",
    "batch_1127__job_job-000224",
    "batch_1152__job_job-000216",
    "batch_1155__job_job-000539",
    "batch_1155__job_job-000572",
    "batch_1155__job_job-000661",
    "batch_1155__job_job-000869",
    "batch_1156__job_job-000311",
    "batch_1156__job_job-000351",
    "batch_1157__job_job-000326",
    "batch_1157__job_job-000340",
    "batch_1159__job_job-000147",
    "batch_1159__job_job-000148",
    "batch_1159__job_job-000149",
    "batch_1159__job_job-000150",
    "batch_1159__job_job-000171",
    "batch_1159__job_job-000554",
    "batch_1159__job_job-000587",
    "batch_1159__job_job-000605",
    "batch_1159__job_job-000723",
    "batch_1159__job_job-000786",
    "batch_1159__job_job-000836",
    "batch_1160__job_job-000042",
    "batch_1160__job_job-000412",
    "batch_1160__job_job-000413",
    "batch_1160__job_job-000570",
    "batch_1160__job_job-000633",
    "batch_1160__job_job-000634",
    "batch_1160__job_job-000774",
    "batch_1161__job_job-000016",
    "batch_1161__job_job-000018",
    "batch_1161__job_job-000019",
    "batch_1161__job_job-000438",
    "batch_1161__job_job-000613",
    "batch_1162__job_job-000144",
    "batch_1162__job_job-000228",
    "batch_1162__job_job-000231",
    "batch_1162__job_job-000276",
    "batch_1162__job_job-000289",
    "batch_1162__job_job-000350",
    "batch_1162__job_job-000863",
    "batch_1162__job_job-000912",
    "batch_1162__job_job-000915",
    "batch_1163__job_job-000153",
    "batch_1163__job_job-000236",
    "batch_1163__job_job-000255",
    "batch_1163__job_job-000428",
    "batch_1163__job_job-000559",
    "batch_1163__job_job-000614",
    "batch_1163__job_job-000903",
    "batch_1164__job_job-000121",
    "batch_1164__job_job-000182",
    "batch_1164__job_job-000196",
    "batch_1164__job_job-000705",
    "batch_1164__job_job-000731",
    "batch_1164__job_job-000734",
    "batch_1165__job_job-000022",
    "batch_1165__job_job-000200",
    "batch_1165__job_job-000239",
    "batch_1165__job_job-000529",
    "batch_1165__job_job-000559",
    "batch_1165__job_job-000616",
    "batch_1166__job_job-000473",
    "batch_1166__job_job-000511",
    "batch_1166__job_job-000767",
    "batch_1166__job_job-000789",
    "batch_1166__job_job-000829",
    "batch_1166__job_job-000892",
    "batch_1167__job_job-000256",
    "batch_1167__job_job-000260",
    "batch_1167__job_job-000509",
    "batch_1167__job_job-000841",
    "batch_1167__job_job-000966",
    "batch_1167__job_job-000967",
    "batch_1168__job_job-000215",
    "batch_1168__job_job-000216",
    "batch_1168__job_job-000572",
    "batch_1168__job_job-000576",
    "batch_1168__job_job-000817",
    "batch_1168__job_job-000845",
    "batch_1169__job_job-000146",
    "batch_1169__job_job-000188",
    "batch_1169__job_job-000252",
    "batch_1169__job_job-000292",
    "batch_1169__job_job-000326",
    "batch_1169__job_job-000336",
    "batch_1169__job_job-000397",
    "batch_1169__job_job-000476",
    "batch_1169__job_job-000477",
    "batch_1169__job_job-000603",
    "batch_1169__job_job-000724",
    "batch_1170__job_job-000015",
    "batch_1170__job_job-000077",
    "batch_1170__job_job-000270",
    "batch_1170__job_job-000313",
    "batch_1170__job_job-000636",
    "batch_1170__job_job-000656",
    "batch_1170__job_job-000781",
    "batch_1171__job_job-000134",
    "batch_1171__job_job-000292",
    "batch_1171__job_job-000547",
    "batch_1171__job_job-000779",
    "batch_1173__job_job-000168",
    "batch_1173__job_job-000169",
    "batch_1173__job_job-000287",
    "batch_1173__job_job-000609",
    "batch_1174__job_job-000194",
    "batch_1174__job_job-000216",
    "batch_1174__job_job-000225",
    "batch_1174__job_job-000496",
    "batch_1174__job_job-000566",
    "batch_1174__job_job-000668",
    "batch_1174__job_job-000723",
    "batch_1174__job_job-000725",
    "batch_1174__job_job-000783",
    "batch_1174__job_job-000905",
    "batch_1175__job_job-000328",
    "batch_1175__job_job-000330",
    "batch_1175__job_job-000431",
    "batch_1175__job_job-000432",
    "batch_1175__job_job-000773",
    "batch_1175__job_job-000774",
    "batch_1175__job_job-000788",
    "batch_1175__job_job-000952",
    "batch_1175__job_job-000953",
    "batch_1175__job_job-000963",
    "batch_1176__job_job-000014",
    "batch_1176__job_job-000288",
    "batch_1176__job_job-000376",
    "batch_1176__job_job-000833",
    "batch_1176__job_job-000918",
    "batch_1176__job_job-000919",
    "batch_1177__job_job-000159",
    "batch_1178__job_job-000083",
    "batch_1178__job_job-000196",
    "batch_1178__job_job-000197",
    "batch_1178__job_job-000313",
    "batch_1178__job_job-000350",
    "batch_1178__job_job-000557",
    "batch_1178__job_job-000560",
    "batch_1178__job_job-000686",
    "batch_1179__job_job-000313",
    "batch_1179__job_job-000325",
    "batch_1179__job_job-000521",
    "batch_1179__job_job-000666",
    "batch_1179__job_job-000766",
    "batch_1179__job_job-000943",
    "batch_1179__job_job-000970",
    "batch_1180__job_job-000259",
    "batch_1180__job_job-000301",
    "batch_1180__job_job-000368",
    "batch_1180__job_job-000508",
    "batch_1180__job_job-000629",
    "batch_1180__job_job-000702",
    "batch_1180__job_job-000703",
    "batch_1180__job_job-000739",
    "batch_1181__job_job-000184",
    "batch_1181__job_job-000198",
    "batch_1181__job_job-000308",
    "batch_1181__job_job-000373",
    "batch_1181__job_job-000493",
    "batch_1181__job_job-000494",
    "batch_1181__job_job-000495",
    "batch_1181__job_job-000910",
    "batch_1181__job_job-000974",
    "batch_1182__job_job-000288",
    "batch_1182__job_job-000399",
    "batch_1182__job_job-000400",
    "batch_1182__job_job-000636",
    "batch_1182__job_job-000647",
    "batch_1182__job_job-000648",
    "batch_1183__job_job-000122",
    "batch_1183__job_job-000154",
    "batch_1183__job_job-000155",
    "batch_1183__job_job-000447",
    "batch_1183__job_job-000494",
    "batch_1183__job_job-000653",
    "batch_1184__job_job-000021",
    "batch_1184__job_job-000077",
    "batch_1184__job_job-000096",
    "batch_1184__job_job-000704",
    "batch_1184__job_job-000736",
    "batch_1184__job_job-000772",
    "batch_1184__job_job-000776",
    "batch_1185__job_job-000087",
    "batch_1185__job_job-000217",
    "batch_1186__job_job-000446",
    "batch_1186__job_job-000456",
    "batch_1186__job_job-000457",
    "batch_1186__job_job-000635",
    "batch_1186__job_job-000892",
    "batch_1186__job_job-000893",
    "batch_1186__job_job-000913",
    "batch_1187__job_job-000229",
    "batch_1187__job_job-000373",
    "batch_1187__job_job-000394",
    "batch_1187__job_job-000860",
    "batch_1187__job_job-000881",
    "batch_1187__job_job-000887",
    "batch_1188__job_job-000121",
    "batch_1188__job_job-000122",
    "batch_1188__job_job-000151",
    "batch_1188__job_job-000856",
    "batch_1188__job_job-000900",
    "batch_1189__job_job-000000",
    "batch_1189__job_job-000016",
    "batch_1189__job_job-000113",
    "batch_1189__job_job-000412",
    "batch_1189__job_job-000455",
    "batch_1190__job_job-000092",
    "batch_1190__job_job-000099",
    "batch_1190__job_job-000168",
    "batch_1190__job_job-000262",
    "batch_1190__job_job-000263",
    "batch_1190__job_job-000264",
    "batch_1190__job_job-000342",
    "batch_1190__job_job-000740",
    "batch_1190__job_job-000745",
    "batch_1190__job_job-000821",
    "batch_1190__job_job-000825",
    "batch_1191__job_job-000040",
    "batch_1191__job_job-000464",
    "batch_1191__job_job-000573",
    "batch_1191__job_job-000579",
    "batch_1191__job_job-000784",
    "batch_1191__job_job-000790",
    "batch_1192__job_job-000080",
    "batch_1192__job_job-000102",
    "batch_1192__job_job-000103",
    "batch_1192__job_job-000535",
    "batch_1192__job_job-000569",
    "batch_1192__job_job-000792",
    "batch_1193__job_job-000008",
    "batch_1193__job_job-000324",
    "batch_1193__job_job-000401",
    "batch_1193__job_job-000433",
    "batch_1193__job_job-000657",
    "batch_1193__job_job-000690",
    "batch_1193__job_job-000754",
    "batch_1194__job_job-000108",
    "batch_1194__job_job-000117",
    "batch_1194__job_job-000407",
    "batch_1194__job_job-000554",
    "batch_1194__job_job-000575",
    "batch_1194__job_job-000579",
    "batch_1195__job_job-000324",
    "batch_1195__job_job-000338",
    "batch_1195__job_job-000730",
    "batch_1195__job_job-000806",
    "batch_1195__job_job-000820",
    "batch_1196__job_job-000140",
    "batch_1196__job_job-000141",
    "batch_1196__job_job-000236",
    "batch_1196__job_job-000543",
    "batch_1196__job_job-000674",
    "batch_1196__job_job-000852",
    "batch_1197__job_job-000464",
    "batch_1197__job_job-000465",
    "batch_1197__job_job-000505",
    "batch_1197__job_job-000852",
    "batch_1197__job_job-000914",
    "batch_1197__job_job-000915",
    "batch_1198__job_job-000461",
    "batch_1198__job_job-000514",
    "batch_1198__job_job-000547",
    "batch_1198__job_job-000671",
    "batch_1198__job_job-000849",
    "batch_1198__job_job-000886",
    "batch_1199__job_job-000010",
    "batch_1199__job_job-000137",
    "batch_1199__job_job-000220",
    "batch_1200__job_job-000161",
    "batch_1200__job_job-000180",
    "batch_1200__job_job-000575",
    "batch_1201__job_job-000867",
    "batch_1201__job_job-000868",
    "batch_1201__job_job-000921",
    "batch_1202__job_job-000723",
    "batch_1202__job_job-000732",
    "batch_1202__job_job-000760",
    "batch_1202__job_job-000997",
    "batch_1203__job_job-000004",
    "batch_1203__job_job-000007",
    "batch_1203__job_job-000117",
    "batch_1203__job_job-000675",
    "batch_1203__job_job-000778",
    "batch_1204__job_job-000076",
    "batch_1204__job_job-000086",
    "batch_1204__job_job-000094",
    "batch_1204__job_job-000553",
    "batch_1204__job_job-000554",
    "batch_1204__job_job-000694",
    "batch_1205__job_job-000622",
    "batch_1205__job_job-000635",
    "batch_1205__job_job-000691",
    "batch_1206__job_job-000332",
    "batch_1206__job_job-000385",
    "batch_1206__job_job-000476",
    "batch_1207__job_job-000062",
    "batch_1207__job_job-000063",
    "batch_1207__job_job-000080",
    "batch_1207__job_job-000759",
    "batch_1209__job_job-000907",
    "batch_1209__job_job-000919",
    "batch_1350__job_job-000280",
    // M4dm4ni4c
    // https://boinc.berkeley.edu/central/show_host_detail.php?hostid=42504
    "batch_1405__job_job-000366",
    "batch_1425__job_job-000211",
    "batch_1426__job_job-000601",
    "batch_1430__job_job-000835",
    "batch_1433__job_job-000413",
    "batch_1433__job_job-000845",
    "batch_1477__job_job-000154",
    "batch_1477__job_job-000466",
    "batch_1477__job_job-000676",
    "batch_1478__job_job-000987",
    "batch_1483__job_job-000473",
    "batch_1492__job_job-000892",
    "batch_1493__job_job-000780",
};

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
            // must be a file
            if !e.file_type().is_file() {
                return false;
            }

            let file_name = match e.path().file_name().and_then(|n| n.to_str()) {
                Some(s) => s,
                None => return false,
            };

            // must end with "output"
            if !file_name.ends_with("output") {
                return false;
            }

            // strip "__file_output" and check ignore set
            if let Some(base) = file_name.strip_suffix("__file_output") {
                if JOBS_TO_IGNORE.contains(base) {
                    return false;
                }
            }

            true
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
