type Function = u32;

const INFINITY: u32 = 0xffffffff;

#[derive(Debug)]
struct Result {
    lengths: Vec<u32>,
    expressions: Vec<Vec<[Function; 2]>>,
    lists: Vec<Vec<Function>>,
    stats: Vec<u32>,
}

fn find_normal_lengths(n: u32, target_functions: &Vec<Function>) -> Result {
    // L1. Initialize.
    let mut result = Result {
        lengths: vec![INFINITY; 2u32.pow(2u32.pow(n) - 1) as usize],
        expressions: vec![vec![]; 2u32.pow(2u32.pow(n) - 1) as usize],
        lists: vec![vec![]; 1],
        stats: vec![0; 1],
    };
    result.lengths[0] = 0;
    result.stats[0] += 1;
    let tautology = 2u32.pow(2u32.pow(n)) - 1;
    for k in 1..=n {
        let slice = 2u32.pow(2u32.pow(n - k)) + 1;
        let f = tautology / slice;
        result.lengths[f as usize] = 0;
        result.lists[0].push(f);
        result.stats[0] += 1;
    }
    // initialize c, the number of functions where L(f) = infinity
    let mut c = 2u32.pow(2u32.pow(n) - 1) - n - 1;

    // L2. Loop over r = 1, 2, ...
    for r in 1.. {
        while result.lists.len() <= r as usize {
            result.lists.push(vec![]);
        }
        while result.stats.len() <= r as usize {
            result.stats.push(0);
        }
        // L3. Loop over j = 0, 1, ..., r - 1 k = r - 1 - j while j <= k
        for j in 0..=r - 1 {
            let k = r - 1 - j;
            if k < j {
                break;
            }
            // L4. Loop over all functions g and h in lists j and k
            for gi in 0..result.lists[j as usize].len() {
                let start_hi = if j == k { gi + 1 } else { 0 };
                for hi in start_hi..result.lists[k as usize].len() {
                    let g = result.lists[j as usize][gi];
                    let h = result.lists[k as usize][hi];
                    // L5. Loop over all new functions f
                    for rf in [g & h, (!g) & h, g & (!h), g | h, g ^ h] {
                        let f = rf & tautology;
                        // L6. Add f if it is new
                        if result.lengths[f as usize] == INFINITY {
                            result.lengths[f as usize] = r;
                            result.expressions[f as usize] = vec![[g, h]];
                            result.lists[r as usize].push(f);
                            result.stats[r as usize] += 1;
                            c -= 1;
                            if c == 0 {
                                for fi in target_functions {
                                    let f = fi & tautology;
                                    println!("{:016b}: {}", f, result.lengths[f as usize]);
                                }
                                return result;
                            }
                        }
                    }
                }
            }
        }
    }

    result
}

fn main() {
    let target_functions: Vec<Function> = vec![
        !0b1011011111100011,
        !0b1111100111100100,
        !0b1101111111110100,
        !0b1011011011011110,
        !0b1010001010111111,
        !0b1000111111110011,
        0b0011111011111111,
    ];
    let result = find_normal_lengths(4, &target_functions);
    println!("{:?}", result.stats);
}
