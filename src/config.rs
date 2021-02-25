use regex::Regex;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct Problem {
    /// 题目名称，如 `math`
    pub name: String,
    #[serde(with = "regex_sd")]
    pub regex: Regex,
    #[serde(default)]
    pub existing_files: Vec<String>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Contestant {
    /// 选手文件夹父路径
    pub root_path: String,
    #[serde(with = "regex_sd")]
    pub regex: Regex,
    /// 所有题目的配置项
    pub problems: Vec<Problem>,
}

mod regex_sd {
    use regex::Regex;
    use serde::Deserialize;
    use std::str::FromStr;

    pub fn serialize<S>(r: &Regex, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&r.to_string())
    }

    pub fn deserialize<'de, D>(deserializer: D) -> Result<Regex, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        Regex::from_str(&s).map_err(serde::de::Error::custom)
    }
}
