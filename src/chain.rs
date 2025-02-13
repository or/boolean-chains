use super::expression::Expression;
use super::function::Function;

use std::collections::HashMap;
use std::collections::HashSet;

pub struct ChainExpression {
    pub index: usize,
    pub function: Function,
    pub expression: Expression,
}

impl ChainExpression {
    pub fn new(index: usize, expression: Expression) -> Self {
        Self {
            index,
            function: expression.evaluate(),
            expression,
        }
    }
}

pub struct Chain {
    pub expressions: Vec<ChainExpression>,
    pub function_lookup: HashMap<Function, usize>,
    pub targets: Vec<Function>,
    pub target_lookup: HashSet<Function>,
}

impl Chain {
    pub fn new(targets: &Vec<Function>) -> Self {
        Self {
            expressions: Vec::new(),
            function_lookup: HashMap::new(),
            targets: targets.clone(),
            target_lookup: targets.iter().cloned().collect(),
        }
    }

    pub fn add(&mut self, expr: Expression) {
        let index = self.expressions.len();
        let f = expr.evaluate();
        self.expressions.push(ChainExpression::new(index, expr));
        self.function_lookup.insert(f, index);
    }

    pub fn get_name(&self, chain_expression: &ChainExpression) -> String {
        format!("x{}", chain_expression.index + 1)
    }

    pub fn get_name_for_function(&self, f: Function) -> String {
        match self.function_lookup.get(&f) {
            Some(index) => self.get_name(&self.expressions[*index]),
            None => "unknown".to_string(),
        }
    }

    pub fn get_expression_as_str(&self, chain_expression: &ChainExpression) -> String {
        let name = self.get_name(chain_expression);
        let is_target = if self.target_lookup.contains(&chain_expression.function) {
            " [target]"
        } else {
            ""
        };

        match chain_expression.expression {
            Expression::Constant(f1) => format!("{name} = {f1}"),
            Expression::And(f1, f2) => {
                format!(
                    "{name} = {} & {} = {}{}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2),
                    chain_expression.function,
                    is_target
                )
            }
            Expression::Or(f1, f2) => {
                format!(
                    "{name} = {} | {} = {}{}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2),
                    chain_expression.function,
                    is_target
                )
            }
            Expression::Xor(f1, f2) => {
                format!(
                    "{name} = {} ^ {} = {}{}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2),
                    chain_expression.function,
                    is_target
                )
            }
            Expression::ButNot(f1, f2) => {
                format!(
                    "{name} = {} > {} = {}{}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2),
                    chain_expression.function,
                    is_target
                )
            }
            Expression::NotBut(f1, f2) => {
                format!(
                    "{name} = {} < {} = {}{}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2),
                    chain_expression.function,
                    is_target
                )
            }
        }
    }

    pub fn print(&self) {
        println!("chain:");
        for chain_expression in &self.expressions {
            println!("  {}", self.get_expression_as_str(chain_expression));
        }
    }
}
