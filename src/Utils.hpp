#include <stdexcept>
#include <string>

// A simple helper to parse circuit values with SI suffixes.
static double parseValue(const std::string& val_str) {
  std::string num_str;
  char suffix = ' ';
  for (char c : val_str) {
    if (std::isdigit(c) || c == '.' || c == '-') {
      num_str += c;
    } else {
      suffix = std::tolower(c);
      break;
    }
  }

  double val = std::stod(num_str);
  switch (suffix) {
    case 'f':
      val *= 1e-15;
      break;
    case 'p':
      val *= 1e-12;
      break;
    case 'n':
      val *= 1e-9;
      break;
    case 'u':
      val *= 1e-6;
      break;
    case 'm':
      val *= 1e-3;
      break;
    case 'k':
      val *= 1e3;
      break;
    case 'M':
      val *= 1e6;
      break;
    case 'G':
      val *= 1e9;
      break;
    default:
      break;
  }
  return val;
}