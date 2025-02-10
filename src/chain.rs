use super::expression::Expression;
use super::function::Function;

use std::collections::HashMap;

pub struct ChainExpression<const N: u32> {
    pub index: usize,
    pub function: Function<N>,
    pub expression: Expression<N>,
}

impl<const N: u32> ChainExpression<N> {
    pub fn new(index: usize, expression: Expression<N>) -> Self {
        Self {
            index,
            function: expression.evaluate(),
            expression,
        }
    }
}

pub struct Chain<const N: u32> {
    pub expressions: Vec<ChainExpression<N>>,
    pub function_lookup: HashMap<Function<N>, usize>,
}

impl<const N: u32> Chain<N> {
    pub fn new() -> Self {
        Self {
            expressions: Vec::new(),
            function_lookup: HashMap::new(),
        }
    }

    pub fn add(&mut self, expr: Expression<N>) {
        let index = self.expressions.len();
        let f = expr.evaluate();
        self.expressions.push(ChainExpression::new(index, expr));
        self.function_lookup.insert(f, index);
    }

    pub fn get_name(&self, chain_expression: &ChainExpression<N>) -> String {
        format!("x{}", chain_expression.index + 1)
    }

    pub fn get_name_for_function(&self, f: Function<N>) -> String {
        match self.function_lookup.get(&f) {
            Some(index) => self.get_name(&self.expressions[*index]),
            None => "unknown".to_string(),
        }
    }

    pub fn get_expression_as_str(&self, chain_expression: &ChainExpression<N>) -> String {
        let name = self.get_name(chain_expression);
        match chain_expression.expression {
            Expression::Constant(f1) => format!("{name} = {f1}"),
            Expression::And(f1, f2) => {
                format!(
                    "{name} = {} & {}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2)
                )
            }
            Expression::Or(f1, f2) => {
                format!(
                    "{name} = {} | {}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2)
                )
            }
            Expression::Xor(f1, f2) => {
                format!(
                    "{name} = {} ^ {}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2)
                )
            }
            Expression::ButNot(f1, f2) => {
                format!(
                    "{name} = {} > {}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2)
                )
            }
            Expression::NotBut(f1, f2) => {
                format!(
                    "{name} = {} < {}",
                    self.get_name_for_function(f1),
                    self.get_name_for_function(f2)
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
