#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <map>
#include <string>
#include <vector>

class printer {
public:
  printer(std::string &output) : indent_(0), output_(output) {}

  void Add(const std::string &lines) {
    //
    // use 2 space indent
    std::string indent_str;
    for (std::size_t i = 0; i < indent_; i++) {
      indent_str += " ";
      indent_str += " ";
    }

    std::vector<std::string> strs;
    boost::split(strs, lines, boost::is_any_of("\n"));
    std::string indented_lines;
    for (auto str : strs) {
      if (str.empty()) {
        continue;
      }
      indented_lines += indent_str + str + "\n";
    }
    output_ += indented_lines;
  }

  void AddLn(const std::string &lines) { Add(lines + "\n"); }

  void Add(const std::map<std::string, std::string> &vars,
           const std::string &lines) {
    // replace all occurances of $var with value in lines before add to output
    std::string replaced_lines = lines;
    for (auto kv : vars) {
      std::string key = "$" + kv.first + "$";
      const std::string &val = kv.second;
      boost::replace_all(replaced_lines, key, val);
    }
    Add(replaced_lines);
  }

  void AddLn(const std::map<std::string, std::string> &vars,
             const std::string &lines) {
    Add(vars, lines + "\n");
  }

  void Indent() { indent_++; }

  void Outdent() {
    assert(indent_ > 0);
    indent_--;
  }

  std::string GetOutput() { return output_; }

private:
  std::size_t indent_;
  std::string &output_;
};