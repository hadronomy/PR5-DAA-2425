#ifndef MITATA
#define MITATA

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "mitata/lib.h"
#include "mitata/types.h"

namespace mitata {

namespace ctx {
inline const std::string compiler() {
#if defined(__APPLE__)
  return "clang (Apple)";

#elif defined(_MSC_VER)
  return "msvc";

#elif defined(__INTEL_COMPILER)
  return "intel c++";

#elif defined(__clang__)
  return "clang (LLVM)";

#elif defined(__GNUC__) || defined(__GNUG__)
  return "gcc";

#else
  return "null";
#endif
}
}  // namespace ctx

namespace fmt {
struct k_bins {
  u64 avg;
  u64 peak;
  u64 outliers;
  f64 min, max, step;
  std::vector<u64> bins;
  std::vector<f64> steps;
};

namespace colors {
static const auto bold = "\x1b[1m";
static const auto reset = "\x1b[0m";

static const auto red = "\x1b[31m";
static const auto cyan = "\x1b[36m";
static const auto blue = "\x1b[34m";
static const auto gray = "\x1b[90m";
static const auto white = "\x1b[37m";
static const auto black = "\x1b[30m";
static const auto green = "\x1b[32m";
static const auto yellow = "\x1b[33m";
static const auto magenta = "\x1b[35m";
}  // namespace colors

inline const std::string str(std::string s, u64 len = 3) {
  if (len >= s.length())
    return s;
  return s.substr(0, len - 2) + "..";
}

inline const std::string pad_e(std::string s, u64 len, char c = ' ') {
  if (s.find("µ") != std::string::npos)
    len += 1;

  if (len <= s.length())
    return s;
  return s.append(len - s.length(), c);
}

inline const std::string pad_s(std::string s, u64 len, char c = ' ') {
  if (s.find("µ") != std::string::npos)
    len += 1;

  if (len <= s.length())
    return s;
  return s.insert(0, len - s.length(), c);
}

inline const std::string time(f64 ns) {
  std::ostringstream buf;
  buf.precision(2);
  buf << std::fixed;
  if (ns < 1e0) {
    buf << ns * 1e3 << " ps";
    return buf.str();
  }
  if (ns < 1e3) {
    buf << ns << " ns";
    return buf.str();
  }
  ns /= 1000;
  if (ns < 1e3) {
    buf << ns << " µs";
    return buf.str();
  }
  ns /= 1000;
  if (ns < 1e3) {
    buf << ns << " ms";
    return buf.str();
  }
  ns /= 1000;

  if (ns < 1e3) {
    buf << ns << " s";
    return buf.str();
  }
  ns /= 60;
  if (ns < 1e3) {
    buf << ns << " m";
    return buf.str();
  }
  buf << (ns / 60) << " h";
  return buf.str();
}

inline const k_bins bins(lib::k_stats stats, u64 size = 6, f64 percentile = 1) {
  u64 poffset = percentile * (stats.samples.size() - 1);
  auto clamp = [](auto m, auto v, auto x) {
    return v < m ? m : v > x ? x : v;
  };

  f64 min = stats.min;
  f64 max =
    .0 != stats.samples[poffset] ? stats.samples[poffset] : (.0 == stats.max ? 1.0 : stats.max);

  f64 step = (max - min) / (size - 1);
  auto bins = std::vector<u64>(size, 0);
  auto steps = std::vector<f64>(size, 0);

  for (auto o = 0; o < size; o++)
    steps[o] = min + o * step;
  for (auto o = 0; o <= poffset; o++)
    bins[std::round((stats.samples[o] - min) / step)]++;

  return {
    .avg = clamp(0, (u64)std::round((stats.avg - min) / step), size - 1),
    .peak = *std::max_element(bins.begin(), bins.end()),
    .outliers = stats.samples.size() - 1 - poffset,
    .min = min,
    .max = max,
    .step = step,
    .bins = bins,
    .steps = steps,
  };
}

inline const std::string
  barplot(std::map<std::string, f64> map, u64 legend = 8, u64 width = 14, bool colors = true) {
  std::string barplot = "";
  f64 min = std::min_element(map.begin(), map.end(), [](const auto& a, const auto& b) {
              return a.second < b.second;
            })->second;
  f64 max = std::max_element(map.begin(), map.end(), [](const auto& a, const auto& b) {
              return a.second > b.second;
            })->second;

  auto steps = width - 11;
  f64 step = (max - min) / step;

  barplot += std::string(1 + legend, ' ');
  barplot += "┌" + std::string(width, ' ') + "┐" + "\n";

  for (const auto& [name, value] : map) {
    u64 offset = 1 + std::round((value - min) / step);
    barplot += fmt::pad_s(fmt::str(name, legend), legend) + " ┤";

    if (colors)
      barplot += fmt::colors::gray;
    for (auto o = 0; o < offset; o++)
      barplot += "■";
    if (colors)
      barplot += fmt::colors::reset;

    barplot += " ";
    if (colors)
      barplot += fmt::colors::yellow;
    barplot += fmt::time(value);
    if (colors)
      barplot += fmt::colors::reset;
    barplot += " \n";
  }

  barplot += std::string(1 + legend, ' ');
  barplot += "└" + std::string(width, ' ') + "┘" + "\n";

  return barplot;
}

inline const std::vector<std::string> histogram(k_bins bins, u64 height = 2, bool colors = true) {
  auto histogram = std::vector<std::string>(height);
  auto clamp = [](auto m, auto v, auto x) {
    return v < m ? m : v > x ? x : v;
  };
  auto symbols = std::vector<std::string>{"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
  auto canvas =
    std::vector<std::vector<std::string>>(height, std::vector<std::string>(bins.bins.size(), " "));

  u64 avg = bins.avg;
  u64 peak = bins.peak;
  f64 scale = (f64)(height * symbols.size() - 1) / peak;

  for (auto o = 0; o < bins.bins.size(); o++) {
    auto b = bins.bins[o];
    auto s = std::round(b * scale);

    for (auto h = 0; h < height; h++) {
      auto leftover = s - symbols.size();
      canvas[h][o] = symbols[clamp(0, s, symbols.size() - 1)];
      if (0 >= (s = leftover))
        break;
    }
  }

  for (auto h = 0; h < height; h++) {
    std::string l = "";

    if (0 != avg) {
      if (colors)
        l += fmt::colors::cyan;
      for (auto o = 0; o < avg; o++)
        l += canvas[h][o];
      if (colors)
        l += fmt::colors::reset;
    }

    if (colors)
      l += fmt::colors::yellow;
    l += canvas[h][avg];
    if (colors)
      l += fmt::colors::reset;

    if (avg != (bins.bins.size() - 1)) {
      if (colors)
        l += fmt::colors::magenta;
      for (auto o = 1 + avg; o < bins.bins.size(); o++)
        l += canvas[h][o];
      if (colors)
        l += fmt::colors::reset;
    }

    histogram[h] = l;
  }

  return (std::reverse(histogram.begin(), histogram.end()), histogram);
}

inline const std::vector<std::string> timeout_histogram(u64 height = 2, bool colors = true) {
  auto histogram = std::vector<std::string>(height);

  for (auto h = 0; h < height; h++) {
    std::string l = "";
    if (colors)
      l += fmt::colors::red;
    l += lib::repeat(height == 1 ? 11 : 21, "🮘");
    if (colors)
      l += fmt::colors::reset;
    histogram[h] = l;
  }

  return histogram;
}

inline const std::string boxplot(
  std::map<std::string, lib::k_stats> map,
  u64 legend = 8,
  u64 width = 14,
  bool colors = true
) {
  std::string boxplot = "";
  f64 tmin = std::min_element(map.begin(), map.end(), [](const auto& a, const auto& b) {
               return a.second.min < b.second.min;
             })->second.min;
  auto tmax_stats =
    std::max_element(map.begin(), map.end(), [](const auto& a, const auto& b) {
      return (.0 != a.second.p99 ? a.second.p99 : (.0 == a.second.max ? 1.0 : a.second.max)) <
             (.0 != b.second.p99 ? b.second.p99 : (.0 == b.second.max ? 1.0 : b.second.max));
    })->second;

  f64 tmax = .0 != tmax_stats.p99 ? tmax_stats.p99 : (.0 == tmax_stats.max ? 1.0 : tmax_stats.max);

  auto steps = 2 + width;
  auto step = (tmax - tmin) / (steps - 1);

  boxplot += std::string(1 + legend, ' ');
  boxplot += "┌" + std::string(width, ' ') + "┐" + "\n";

  for (const auto& [name, stats] : map) {
    f64 min = stats.min;
    f64 avg = stats.avg;
    f64 p25 = stats.p25;
    f64 p75 = stats.p75;
    f64 max = .0 != stats.p99 ? stats.p99 : (.0 == stats.max ? 1.0 : stats.max);

    u64 min_offset = std::round((min - tmin) / step);
    u64 max_offset = std::round((max - tmin) / step);
    u64 avg_offset = std::round((avg - tmin) / step);
    u64 p25_offset = std::round((p25 - tmin) / step);
    u64 p75_offset = std::round((p75 - tmin) / step);

    auto u = std::vector<std::string>(1 + max_offset, " ");
    auto m = std::vector<std::string>(1 + max_offset, " ");
    auto l = std::vector<std::string>(1 + max_offset, " ");

    if (min_offset < p25_offset) {
      u[min_offset] = (!colors ? "╷" : (std::string(fmt::colors::cyan) + "╷" + fmt::colors::reset));
      m[min_offset] = (!colors ? "├" : (std::string(fmt::colors::cyan) + "├" + fmt::colors::reset));
      l[min_offset] = (!colors ? "╵" : (std::string(fmt::colors::cyan) + "╵" + fmt::colors::reset));
      for (auto o = 1 + min_offset; o < p25_offset; o++)
        m[o] = (!colors ? "─" : (std::string(fmt::colors::cyan) + "─" + fmt::colors::reset));
    }

    if (p25_offset < avg_offset) {
      u[p25_offset] = (!colors ? "┌" : (std::string(fmt::colors::cyan) + "┌" + fmt::colors::reset));
      l[p25_offset] = (!colors ? "└" : (std::string(fmt::colors::cyan) + "└" + fmt::colors::reset));

      auto ms = min_offset == p25_offset ? "│" : "┤";
      m[p25_offset] = (!colors ? ms : (std::string(fmt::colors::cyan) + ms + fmt::colors::reset));
      for (auto o = 1 + p25_offset; o < avg_offset; o++)
        u[o] = l[o] = (!colors ? "─" : (std::string(fmt::colors::cyan) + "─" + fmt::colors::reset));
    }

    u[avg_offset] = (!colors ? "┬" : (std::string(fmt::colors::yellow) + "┬" + fmt::colors::reset));
    m[avg_offset] = (!colors ? "│" : (std::string(fmt::colors::yellow) + "│" + fmt::colors::reset));
    l[avg_offset] = (!colors ? "┴" : (std::string(fmt::colors::yellow) + "┴" + fmt::colors::reset));

    if (p75_offset > avg_offset) {
      u[p75_offset] =
        (!colors ? "┐" : (std::string(fmt::colors::magenta) + "┐" + fmt::colors::reset));
      l[p75_offset] =
        (!colors ? "┘" : (std::string(fmt::colors::magenta) + "┘" + fmt::colors::reset));

      auto ms = max_offset == p75_offset ? "│" : "├";
      m[p75_offset] =
        (!colors ? ms : (std::string(fmt::colors::magenta) + ms + fmt::colors::reset));
      for (auto o = 1 + avg_offset; o < p75_offset; o++)
        u[o] = l[o] =
          (!colors ? "─" : (std::string(fmt::colors::magenta) + "─" + fmt::colors::reset));
    }

    if (max_offset > p75_offset) {
      u[max_offset] =
        (!colors ? "╷" : (std::string(fmt::colors::magenta) + "╷" + fmt::colors::reset));
      m[max_offset] =
        (!colors ? "┤" : (std::string(fmt::colors::magenta) + "┤" + fmt::colors::reset));
      l[max_offset] =
        (!colors ? "╵" : (std::string(fmt::colors::magenta) + "╵" + fmt::colors::reset));
      for (auto o = 1 + std::max(avg_offset, p75_offset); o < max_offset; o++)
        m[o] = (!colors ? "─" : (std::string(fmt::colors::magenta) + "─" + fmt::colors::reset));
    }

    boxplot += std::string(1 + legend, ' ');
    for (auto s : u)
      boxplot += s;
    boxplot += "\n";
    boxplot += fmt::pad_s(fmt::str(name, legend), legend);
    boxplot += " ";
    for (auto s : m)
      boxplot += s;
    boxplot += "\n";
    boxplot += std::string(1 + legend, ' ');
    for (auto s : l)
      boxplot += s;
    boxplot += "\n";
  }

  boxplot += std::string(1 + legend, ' ');
  boxplot += "└" + std::string(width, ' ') + "┘" + "\n";

  auto min = fmt::time(tmin);
  auto max = fmt::time(tmax);
  auto mid = fmt::time((tmin + tmax) / 2);

  auto u_fix = 0 + (min.find("µ") != std::string::npos ? 1 : 0) +
               (mid.find("µ") != std::string::npos ? 1 : 0) +
               (max.find("µ") != std::string::npos ? 1 : 0);

  f64 gap = (f64)(width + u_fix - min.length() - mid.length() - max.length()) / 2;

  boxplot += std::string(1 + legend, ' ');

  if (colors)
    boxplot += fmt::colors::cyan;
  boxplot += min;
  if (colors)
    boxplot += fmt::colors::reset;

  boxplot += std::string(std::floor(gap), ' ') + ' ';

  if (colors)
    boxplot += fmt::colors::gray;
  boxplot += mid;
  if (colors)
    boxplot += fmt::colors::reset;

  boxplot += ' ' + std::string(std::ceil(gap), ' ');

  if (colors)
    boxplot += fmt::colors::magenta;
  boxplot += max;
  if (colors)
    boxplot += fmt::colors::reset;

  return boxplot + "\n";
}

inline const std::string lineplot(
  std::map<std::string, std::map<double, double>> data_series,
  u64 legend = 8,
  u64 width = 30,
  u64 height = 10,
  bool colors = true
) {
  std::string lineplot = "";

  // Find global min/max for x and y values
  double x_min = std::numeric_limits<double>::max();
  double x_max = std::numeric_limits<double>::lowest();
  double y_min = std::numeric_limits<double>::max();
  double y_max = std::numeric_limits<double>::lowest();

  // Collect all points for better scaling
  std::vector<std::pair<double, double>> all_points;
  for (const auto& [series_name, points] : data_series) {
    for (const auto& [x, y] : points) {
      all_points.push_back({x, y});
      x_min = std::min(x_min, x);
      x_max = std::max(x_max, x);
      // For log scale, we need positive values
      if (y > 0) {
        y_min = std::min(y_min, y);
        y_max = std::max(y_max, y);
      }
    }
  }

  // Handle case when no valid positive values
  if (y_min <= 0 || y_max <= 0) {
    y_min = 1;
    y_max = 10;
  }

  // Convert to log scale
  double log_y_min = std::log10(y_min);
  double log_y_max = std::log10(y_max);

  // Better scale adjustment for logarithmic scale
  double log_y_range = log_y_max - log_y_min;
  double padding = log_y_range * 0.1;
  log_y_min -= padding;
  log_y_max += padding;
  log_y_range = log_y_max - log_y_min;

  // Improved scale calculations
  double x_scale =
    (width > 1) ? (width - 1) / std::max(x_max - x_min, std::numeric_limits<double>::epsilon()) : 0;
  double y_scale = (height > 1) ? (height - 1) / log_y_range : 0;

  // Create a 2D grid for each series
  std::map<std::string, std::vector<std::vector<bool>>> grids;
  for (const auto& [series_name, points] : data_series) {
    auto& grid = grids[series_name] =
      std::vector<std::vector<bool>>(height, std::vector<bool>(width, false));

    // Sort points by x value to connect them properly
    std::vector<std::pair<double, double>> sorted_points;
    for (const auto& [x, y] : points) {
      if (y > 0) {  // Only include positive values for log scale
        sorted_points.push_back({x, y});
      }
    }
    std::sort(sorted_points.begin(), sorted_points.end());

    // Improved point plotting with better interpolation
    for (size_t i = 0; i < sorted_points.size() - 1; i++) {
      double x1 = sorted_points[i].first;
      double y1 = sorted_points[i].second;
      double x2 = sorted_points[i + 1].first;
      double y2 = sorted_points[i + 1].second;

      // More precise grid coordinate calculation with log scale for y
      int grid_x1 = std::clamp(
        static_cast<int>(std::round((x1 - x_min) * x_scale)), 0, static_cast<int>(width - 1)
      );
      int grid_y1 = std::clamp(
        static_cast<int>(height - 1 - std::round((std::log10(y1) - log_y_min) * y_scale)),
        0,
        static_cast<int>(height - 1)
      );
      int grid_x2 = std::clamp(
        static_cast<int>(std::round((x2 - x_min) * x_scale)), 0, static_cast<int>(width - 1)
      );
      int grid_y2 = std::clamp(
        static_cast<int>(height - 1 - std::round((std::log10(y2) - log_y_min) * y_scale)),
        0,
        static_cast<int>(height - 1)
      );

      // Bresenham's line algorithm
      int dx = std::abs(grid_x2 - grid_x1);
      int dy = std::abs(grid_y2 - grid_y1);
      int sx = grid_x1 < grid_x2 ? 1 : -1;
      int sy = grid_y1 < grid_y2 ? 1 : -1;
      int err = dx - dy;

      while (true) {
        if (grid_x1 >= 0 && grid_x1 < static_cast<int>(width) && grid_y1 >= 0 &&
            grid_y1 < static_cast<int>(height)) {
          grid[grid_y1][grid_x1] = true;
        }

        if (grid_x1 == grid_x2 && grid_y1 == grid_y2)
          break;
        int e2 = 2 * err;
        if (e2 > -dy) {
          err -= dy;
          grid_x1 += sx;
        }
        if (e2 < dx) {
          err += dx;
          grid_y1 += sy;
        }
      }
    }
  }

  // Define series line symbols and colors
  std::vector<std::pair<std::string, std::string>> series_styles;
  series_styles.push_back({colors ? fmt::colors::cyan : "", "●"});
  series_styles.push_back({colors ? fmt::colors::yellow : "", "■"});
  series_styles.push_back({colors ? fmt::colors::magenta : "", "▲"});
  series_styles.push_back({colors ? fmt::colors::green : "", "◆"});
  series_styles.push_back({colors ? fmt::colors::red : "", "+"});
  series_styles.push_back({colors ? fmt::colors::blue : "", "×"});

  // Draw the plot frame with proper y-axis labels
  lineplot += std::string(1 + legend, ' ');
  lineplot += "+" + std::string(width, '-') + "+" + "\n";

  // Draw plot body with data
  for (u64 y = 0; y < height; y++) {
    std::vector<std::string> line(width, " ");

    // Fill in series data points
    size_t series_idx = 0;
    for (const auto& [series_name, grid] : grids) {
      for (u64 x = 0; x < width; x++) {
        if (grid[y][x]) {
          auto [color, symbol] = series_styles[series_idx % series_styles.size()];
          line[x] = color + symbol + (colors ? fmt::colors::reset : "");
        }
      }
      series_idx++;
    }

    // Improved y-axis label formatting with logarithmic scale
    std::string y_label = "";
    if (y == 0 || y == height - 1 || y % (height / 4) == 0) {
      double log_y_value = log_y_max - (y * log_y_range / (height - 1));
      double y_value = std::pow(10, log_y_value);
      y_label = fmt::pad_s(fmt::time(y_value), legend);
    }

    lineplot += fmt::pad_e(y_label, legend) + " │";
    for (const auto& cell : line) {
      lineplot += cell;
    }
    lineplot += "│\n";
  }

  // Draw bottom border
  lineplot += std::string(1 + legend, ' ');
  lineplot += "└" + std::string(width, '-') + "┘" + "\n";

  // Improved x-axis label formatting
  lineplot += std::string(1 + legend, ' ') + " ";
  auto x_min_str = fmt::time(x_min);  // Use time formatting for x-axis
  auto x_max_str = fmt::time(x_max);
  lineplot += x_min_str;
  lineplot += std::string(width - x_min_str.length() - x_max_str.length() - 2, ' ');
  lineplot += x_max_str + "\n";

  // Improved legend with better spacing
  lineplot += "\n";
  size_t series_idx = 0;
  size_t max_name_length = 0;

  // Find longest name for alignment
  for (const auto& [series_name, _] : data_series) {
    max_name_length = std::max(max_name_length, series_name.length());
  }

  // Draw legend with proper alignment
  for (const auto& [series_name, _] : data_series) {
    auto [color, symbol] = series_styles[series_idx % series_styles.size()];
    lineplot += " " + (colors ? color : "") + symbol + (colors ? fmt::colors::reset : "") + " ";
    lineplot += fmt::pad_e(series_name, max_name_length + 2) + "\n";
    series_idx++;
  }

  return lineplot;
}

inline const std::string lineplot(
  std::map<std::string, std::vector<double>> data_series,
  u64 legend = 8,
  u64 width = 30,
  u64 height = 10,
  bool colors = true
) {
  // Convert vector series to map series
  std::map<std::string, std::map<double, double>> converted_series;
  for (const auto& [series_name, values] : data_series) {
    std::map<double, double> point_map;
    for (size_t i = 0; i < values.size(); i++) {
      point_map[static_cast<double>(i)] = values[i];
    }
    converted_series[series_name] = point_map;
  }

  return lineplot(converted_series, legend, width, height, colors);
}

struct visualization_options {
  bool colors = true;
  u64 width = 14;
  u64 legend = 8;
  bool show_summary = true;
  bool show_boxplot = true;
  bool show_barplot = true;
  bool show_lineplot = false;
};

}  // namespace fmt

enum class FunctionKind { None, Function, Generator, Iterator };

template <typename T>
void do_not_optimize(T const& value) {
#if defined(__clang__)
  asm volatile("" : : "r,m"(value) : "memory");
#else
  asm volatile("" : : "r"(value) : "memory");
#endif
}

// Add result type to store benchmark results
struct BenchmarkResult {
  std::map<std::string, double> values;

  // Helper methods for reporting results
  void report(const std::string& key, double value) { values[key] = value; }

  double get(const std::string& key, double default_value = 0.0) const {
    auto it = values.find(key);
    return (it != values.end()) ? it->second : default_value;
  }

  bool has(const std::string& key) const { return values.find(key) != values.end(); }

  // Clear all results
  void clear() { values.clear(); }
};

template <typename F>
FunctionKind get_function_kind(F&& _) {
  if constexpr (std::is_invocable_v<F, std::map<std::string, double>, BenchmarkResult&>) {
    return FunctionKind::Function;
  } else if constexpr (std::is_invocable_v<F, std::map<std::string, double>>) {
    return FunctionKind::Function;
  } else if constexpr (std::is_invocable_v<F>) {
    return FunctionKind::Function;
  }
  return FunctionKind::None;
}

namespace scores {
// Lower time is better (default scoring)
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  time() {
  return
    [](const lib::k_stats& stats, const std::map<std::string, double>&, const BenchmarkResult&) {
      return stats.avg;
    };
}

// Higher is better - invert time
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  inverse_time() {
  return
    [](const lib::k_stats& stats, const std::map<std::string, double>&, const BenchmarkResult&) {
      return stats.avg > 0 ? 1000000.0 / stats.avg : 0.0;
    };
}

// Operations per second - higher is better
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  ops_per_second() {
  return
    [](const lib::k_stats& stats, const std::map<std::string, double>&, const BenchmarkResult&) {
      return stats.avg > 0 ? 1000000000.0 / stats.avg : 0.0;
    };
}

// Custom score function
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  custom(
    std::function<
      double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)> fn
  ) {
  return fn;
}

// Score based on a specific result value
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  result_value(const std::string& key, bool higher_is_better = true) {
  return [key, higher_is_better](
           const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult& result
         ) {
    double value = result.get(key, 0.0);
    return higher_is_better ? value : -value;
  };
}
}  // namespace scores

class B {
  std::string _name;
  bool _highlight = false;
  std::string _highlight_color;
  FunctionKind _kind = FunctionKind::None;
  std::function<
    double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
    _score_fn;

 public:
  std::function<void(std::map<std::string, double>, BenchmarkResult&)> fn;
  bool _compact = false;
  bool _baseline = false;
  std::map<std::string, std::vector<double>> _args;
  BenchmarkResult _last_result;

  B(std::string name, std::function<void(std::map<std::string, double>, BenchmarkResult&)> fn)
      : _name(name), _kind(get_function_kind(fn)), fn(fn) {
    if (_kind == FunctionKind::None) {
      throw std::runtime_error("expected function, generator or iterator");
    }
  }

  // For backward compatibility
  B(std::string name, std::function<void(std::map<std::string, double>)> legacy_fn)
      : _name(name), _kind(get_function_kind(legacy_fn)) {
    if (_kind == FunctionKind::None) {
      throw std::runtime_error("expected function, generator or iterator");
    }

    // Adapt legacy function to new signature
    fn = [legacy_fn](std::map<std::string, double> args, BenchmarkResult&) {
      legacy_fn(args);
    };
  }

  B& score(
    std::function<
      double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
      score_fn
  ) {
    _score_fn = score_fn;
    return *this;
  }

  // Helper method to calculate score if a scoring function is set
  double calculate_score(const lib::k_stats& stats, const std::map<std::string, double>& args)
    const {
    if (_score_fn) {
      return _score_fn(stats, args, _last_result);
    }
    // Default scoring uses average time (lower is better)
    return stats.avg;
  }

  // Is scoring enabled
  bool has_score_function() const { return static_cast<bool>(_score_fn); }

  // Name method remains the same
  B& name(const std::string& new_name, const std::string& color = "") {
    _name = new_name;
    if (!color.empty()) {
      _highlight = true;
      _highlight_color = color;
    }
    return *this;
  }

  B& highlight(const std::string& color = "") {
    _highlight = !color.empty();
    _highlight_color = color;
    return *this;
  }

  B& range(const std::string& name, double start, double end, double multiplier = 8) {
    std::vector<double> values;
    for (double i = start; i <= end; i *= multiplier) {
      values.push_back(std::min(i, end));
    }
    if (values.back() != end) {
      values.push_back(end);
    }
    return args(name, values);
  }

  B& dense_range(const std::string& name, double start, double end, double step = 1) {
    std::vector<double> values;
    for (double i = start; i <= end; i += step) {
      values.push_back(i);
    }
    if (values.back() != end) {
      values.push_back(end);
    }
    return args(name, values);
  }

  B& args(const std::string& param_name, const std::vector<double>& values) {
    _args[param_name] = values;
    return *this;
  }

  // Get formatted name with parameter value substitutions
  std::string get_formatted_name(const std::map<std::string, double>& args) const {
    std::string result = _name;
    for (const auto& [param_name, value] : args) {
      std::string placeholder = "$" + param_name;
      size_t pos = result.find(placeholder);
      if (pos != std::string::npos) {
        result.replace(pos, placeholder.length(), std::to_string(static_cast<int>(value)));
      }
    }
    return result;
  }

  // Get args
  const std::map<std::string, std::vector<double>>& get_args() const { return _args; }

  void compact(bool compact = true) { this->_compact = compact; }

  void baseline(bool baseline = true) { this->_baseline = baseline; }

  // Get name
  const std::string& get_name() const { return _name; }

  // Get highlight status
  bool is_highlighted() const { return _highlight; }

  // Get highlight color
  const std::string& get_highlight_color() const { return _highlight_color; }

  std::string get_base_name() const {
    size_t param_pos = _name.find("(");
    return param_pos != std::string::npos ? _name.substr(0, param_pos) : _name;
  }

  bool has_template_params() const {
    for (const auto& [param_name, _] : _args) {
      if (_name.find("$" + param_name) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  // Get display name for a parameter value
  std::string get_param_display(const std::string& param_name, double value) const {
    if (!has_template_params()) {
      // If using template syntax, display just the value
      return std::to_string(static_cast<int>(value));
    } else {
      // Otherwise use param=value format
      return param_name + "=" + std::to_string(static_cast<int>(value));
    }
  }

  // Get the last result
  const BenchmarkResult& get_last_result() const { return _last_result; }

  // Reset the last result
  void reset_result() { _last_result.clear(); }
};

struct k_run {
  bool colors = true;
  std::string format = "mitata";
  std::regex filter = std::regex(".*");
  u64 timelimit_ns = 0;
};

struct k_collection {
  std::vector<char> types;
  std::map<std::string, B> benchmarks;
};

class runner {
  std::vector<k_collection> collections;
  std::map<std::string, BenchmarkResult> bench_results;

 public:
  runner() { collections.push_back({}); }

  ~runner() { timeout::cleanup(); }

  B* bench(
    const std::string name,
    std::function<void(std::map<std::string, double>, BenchmarkResult&)> fn
  ) {
    return &collections.back().benchmarks.emplace(name, B(name, fn)).first->second;
  }

  // For backward compatibility
  B* bench(const std::string name, std::function<void(std::map<std::string, double>)> legacy_fn) {
    return &collections.back().benchmarks.emplace(name, B(name, legacy_fn)).first->second;
  }

  void group(std::function<void()> fn) {
    auto last = &collections.back();
    if (!last->types.empty())
      last->types.push_back('g');
    else
      collections.push_back({.types = {'g'}});

    fn();
    collections.push_back({});
  }

  void boxplot(std::function<void()> fn) {
    auto last = &collections.back();
    if (!last->types.empty())
      last->types.push_back('x');
    else
      collections.push_back({.types = {'x'}});

    fn();
    collections.push_back({});
  }

  void barplot(std::function<void()> fn) {
    auto last = &collections.back();
    if (!last->types.empty())
      last->types.push_back('b');
    else
      collections.push_back({.types = {'b'}});

    fn();
    collections.push_back({});
  }

  void summary(std::function<void()> fn) {
    auto last = &collections.back();
    if (!last->types.empty())
      last->types.push_back('s');
    else
      collections.push_back({.types = {'s'}});

    fn();
    collections.push_back({});
  }

  void lineplot(std::function<void()> fn) {
    auto last = &collections.back();
    if (!last->types.empty())
      last->types.push_back('l');
    else
      collections.push_back({.types = {'l'}});

    fn();
    collections.push_back({});
  }

  std::map<std::string, lib::k_stats> run(const k_run opts = k_run()) {
    std::map<std::string, lib::k_stats> stats;

    // Fix noop measurement
    lib::k_stats noop = lib::fn([]() { do_not_optimize(0); });

    if ("quiet" == opts.format) {
      for (const auto& collection : collections) {
        for (const auto& [name, bench] : collection.benchmarks) {
          if (!std::regex_match(name, opts.filter))
            continue;
          if (bench._args.empty()) {
            // Reset result for this benchmark
            auto& b = const_cast<B&>(bench);
            b.reset_result();

            // Run with empty args and collect result
            stats[name] =
              lib::fn([&b]() { b.fn({}, b._last_result); }, lib::k_options(), opts.timelimit_ns);

            // Store the result
            bench_results[name] = b._last_result;
          } else {
            for (const auto& [param_name, values] : bench._args) {
              for (const auto& value : values) {
                std::map<std::string, double> args;
                args[param_name] = value;
                std::string formatted_name = bench.get_formatted_name(args);

                // Reset result for this benchmark run
                auto& b = const_cast<B&>(bench);
                b.reset_result();

                // Run with args and collect result
                stats[formatted_name] = lib::fn(
                  [&b, &args]() { b.fn(args, b._last_result); }, lib::k_options(), opts.timelimit_ns
                );

                // Store the result
                bench_results[formatted_name] = b._last_result;
              }
            }
          }
        }
      }
    }

    if ("json" == opts.format) {
      std::cout << "{" << std::endl;
      std::cout << "\"context\": {" << std::endl;
      std::cout << "\"runtime\": \"c++\"," << std::endl;
      std::cout << "\"compiler\": \"" << ctx::compiler() << "\"," << std::endl;

      std::cout << "\"noop\": {" << std::endl;
      std::cout << "\"min\": " << noop.min << "," << std::endl;
      std::cout << "\"max\": " << noop.max << "," << std::endl;
      std::cout << "\"avg\": " << noop.avg << "," << std::endl;
      std::cout << "\"p25\": " << noop.p25 << "," << std::endl;
      std::cout << "\"p50\": " << noop.p50 << "," << std::endl;
      std::cout << "\"p75\": " << noop.p75 << "," << std::endl;
      std::cout << "\"p99\": " << noop.p99 << "," << std::endl;
      std::cout << "\"p999\": " << noop.p999 << "," << std::endl;
      std::cout << "\"ticks\": " << noop.ticks << "," << std::endl;

      std::cout << "\"samples\": [" << std::endl;

      std::cout << noop.samples[0];
      for (auto o = 1; o < noop.samples.size(); o++)
        std::cout << "," << noop.samples[o];
      std::cout << "]" << std::endl << "}" << std::endl;
      std::cout << "}" << "," << std::endl;

      std::cout << "\"benchmarks\": [" << std::endl;
      auto size = std::accumulate(collections.begin(), collections.end(), 0, [](auto a, auto b) {
        return a + b.benchmarks.size();
      });

      auto o = 0;

      for (const auto& collection : collections) {
        for (const auto& [name, bench] : collection.benchmarks) {
          if (!std::regex_match(name, opts.filter))
            continue;

          if (bench._args.empty()) {
            // Reset result for this benchmark
            auto& b = const_cast<B&>(bench);
            b.reset_result();

            // Run with empty args and collect result
            auto wrapped_fn = [&b]() {
              b.fn({}, b._last_result);
            };
            auto s = stats[name] = lib::measure_function(wrapped_fn, opts.timelimit_ns);

            // Store the result
            bench_results[name] = b._last_result;

            std::cout << "{" << std::endl;
            std::cout << "\"name\": \"" << name << "\"," << std::endl;
            std::cout << "\"timeout\": " << (s.timeout ? "true" : "false") << "," << std::endl;

            if (!s.timeout) {
              std::cout << "\"min\": " << s.min << "," << std::endl;
              std::cout << "\"max\": " << s.max << "," << std::endl;
              std::cout << "\"avg\": " << s.avg << "," << std::endl;
              std::cout << "\"p25\": " << s.p25 << "," << std::endl;
              std::cout << "\"p50\": " << s.p50 << "," << std::endl;
              std::cout << "\"p75\": " << s.p75 << "," << std::endl;
              std::cout << "\"p99\": " << s.p99 << "," << std::endl;
              std::cout << "\"p999\": " << s.p999 << "," << std::endl;
              std::cout << "\"ticks\": " << s.ticks << "," << std::endl;

              std::cout << "\"samples\": [" << std::endl;
              if (!s.samples.empty()) {
                std::cout << s.samples[0];
                for (auto o = 1; o < s.samples.size(); o++)
                  std::cout << "," << s.samples[o];
              }
              std::cout << "]" << std::endl;
            }
            std::cout << "}" << std::endl;
            if (++o != size)
              std::cout << "," << std::endl;
          } else {
            // Run parameterized benchmark
            for (const auto& [param, values] : bench._args) {
              for (const auto& value : values) {
                std::string param_name = name + "(" + param + "=" + std::to_string(value) + ")";
                std::map<std::string, double> args;
                args[param] = value;

                // Reset result for this benchmark run
                auto& b = const_cast<B&>(bench);
                b.reset_result();

                auto param_fn = [&b, &args]() {
                  b.fn(args, b._last_result);
                };
                auto s = stats[param_name] = lib::measure_function(param_fn, opts.timelimit_ns);

                // Store the result
                bench_results[param_name] = b._last_result;

                std::cout << "{" << std::endl;
                std::cout << "\"name\": \"" << param_name << "\"," << std::endl;
                std::cout << "\"timeout\": " << (s.timeout ? "true" : "false") << "," << std::endl;

                if (!s.timeout) {
                  std::cout << "\"min\": " << s.min << "," << std::endl;
                  std::cout << "\"max\": " << s.max << "," << std::endl;
                  std::cout << "\"avg\": " << s.avg << "," << std::endl;
                  std::cout << "\"p25\": " << s.p25 << "," << std::endl;
                  std::cout << "\"p50\": " << s.p50 << "," << std::endl;
                  std::cout << "\"p75\": " << s.p75 << "," << std::endl;
                  std::cout << "\"p99\": " << s.p99 << "," << std::endl;
                  std::cout << "\"p999\": " << s.p999 << "," << std::endl;
                  std::cout << "\"ticks\": " << s.ticks << "," << std::endl;

                  std::cout << "\"samples\": [" << std::endl;

                  std::cout << s.samples[0];
                  for (auto o = 1; o < s.samples.size(); o++)
                    std::cout << "," << s.samples[o];

                  std::cout << "]" << std::endl;
                }
                std::cout << "}" << std::endl;
                if (++o != size)
                  std::cout << "," << std::endl;
              }
            }
          }
        }
      }

      std::cout << "]" << std::endl;
      std::cout << "}" << std::endl;
    }

    if ("mitata" == opts.format) {
      const auto k_legend = 28;
      if (opts.colors)
        std::cout << fmt::colors::gray;

      std::cout << "runtime: c++" << std::endl;
      std::cout << "compiler: " << ctx::compiler() << std::endl;

      if (opts.colors)
        std::cout << fmt::colors::reset;

      std::cout << std::endl;
      std::cout << fmt::pad_e("benchmark", k_legend);
      std::cout << "avg (min … max) p75   p99    (min … top 1%)" << std::endl;

      bool first = true;
      bool optimized_out_warning = false;
      for (auto o = 0; o < (15 + k_legend); o++)
        std::cout << "-";
      std::cout << " ";
      for (auto o = 0; o < 31; o++)
        std::cout << "-";
      std::cout << std::endl;

      for (const auto& collection : collections) {
        if (collection.benchmarks.empty())
          continue;

        // Group benchmarks by base name (algorithm without parameters)
        std::map<std::string, std::vector<std::pair<std::string, B*>>> grouped_benchmarks;
        for (const auto& [name, bench] : collection.benchmarks) {
          if (!std::regex_match(name, opts.filter))
            continue;

          std::string base_name = bench.get_base_name();
          grouped_benchmarks[base_name].push_back({name, const_cast<B*>(&bench)});
        }

        std::vector<std::pair<std::string, std::pair<bool, lib::k_stats>>> trials;

        bool has_matches = !grouped_benchmarks.empty();

        if (!has_matches)
          continue;
        else if (first)
          first = false;
        else {
          std::cout << std::endl;
          if (opts.colors)
            std::cout << fmt::colors::gray;
          for (auto o = 0; o < (15 + k_legend); o++)
            std::cout << "-";
          std::cout << " ";
          for (auto o = 0; o < 31; o++)
            std::cout << "-";
          std::cout << (!opts.colors ? "" : fmt::colors::reset) << std::endl;
        }

        // Process each group of benchmarks
        for (const auto& [base_name, benchmarks] : grouped_benchmarks) {
          bool show_header = true;

          // Print group header if multiple benchmarks in group
          if (show_header) {
            auto fname = fmt::pad_e(fmt::str(base_name, k_legend), k_legend);
            std::cout << fname << " " << std::endl;
          }

          if (show_header) {
            if (opts.colors)
              std::cout << fmt::colors::gray;
            for (auto o = 0; o < (k_legend); o++)
              std::cout << "-";
            std::cout << (!opts.colors ? "" : fmt::colors::reset) << std::endl;
          }

          // Process each benchmark in the group
          for (const auto& [name, bench_ptr] : benchmarks) {
            const auto& bench = *bench_ptr;

            if (bench._args.empty()) {
              // Reset result for this benchmark
              auto& b = const_cast<B&>(bench);
              b.reset_result();

              // Run normal benchmark with empty args map and timelimit
              auto wrapped_fn = [&b]() {
                b.fn({}, b._last_result);
              };
              auto s = stats[name] = lib::measure_function(wrapped_fn, opts.timelimit_ns);
              trials.push_back(std::make_pair(name, std::make_pair(bench._baseline, s)));

              // Store the result
              bench_results[name] = b._last_result;

              auto compact = bench._compact;
              bool optimized_out = !s.timeout && s.avg < (1.21 * noop.avg);
              optimized_out_warning = optimized_out_warning || optimized_out;

              // Adjust name display for grouped benchmarks
              std::string display_name = name;
              if (show_header) {
                display_name = "  " + name.substr(base_name.length());
                // Ensure that the display name is not empty after removing the base name
                if (display_name.empty()) {
                  display_name = "  (default)";  // Or some other suitable default
                }
              }

              auto fname = fmt::pad_e(fmt::str(display_name, k_legend), k_legend);

              if (compact) {
                std::cout << fname << " ";

                // Handle timeout display
                if (s.timeout) {
                  if (opts.colors) {
                    std::cout << fmt::colors::red << "TIMEOUT" << fmt::colors::reset;
                    auto p75 = fmt::pad_s("??", 9);
                    auto p99 = fmt::pad_s("??", 9);
                    std::cout << " " << p75 << " " << p99 << " ";
                    // Create the histogram before using it
                    auto timeout_histogram_display = fmt::timeout_histogram(1, opts.colors);
                    std::cout << timeout_histogram_display[0];
                  } else {
                    std::cout << "TIMEOUT ? ?/iter ? ? ? ? " << std::string(11, '#');
                  }
                } else {
                  auto avg = fmt::pad_s(fmt::time(s.avg), 9);

                  if (!opts.colors)
                    std::cout << avg << "/iter";
                  else
                    std::cout << fmt::colors::bold << fmt::colors::yellow << avg
                              << fmt::colors::reset << fmt::colors::bold << "/iter"
                              << fmt::colors::reset;

                  std::cout << " ";
                  auto p75 = fmt::pad_s(fmt::time(s.p75), 9);
                  auto p99 = fmt::pad_s(fmt::time(s.p99), 9);
                  auto histogram = fmt::histogram(fmt::bins(s, 11, .99), 1, opts.colors);

                  if (!opts.colors)
                    std::cout << p75 << " " << p99 << " " << histogram[0];
                  else
                    std::cout << fmt::colors::gray << p75 << " " << p99 << fmt::colors::reset << " "
                              << histogram[0];
                  if (optimized_out)
                    std::cout << " " << (!opts.colors ? "" : fmt::colors::red) << "!"
                              << (!opts.colors ? "" : fmt::colors::reset);
                }
              } else {
                std::cout << fname << " ";

                // Handle timeout display for non-compact mode
                if (s.timeout) {
                  auto timeout_histogram_display = fmt::timeout_histogram(2, opts.colors);
                  if (opts.colors) {
                    // Fix alignment by using proper padding
                    std::cout << fmt::colors::red << fmt::pad_s("TIMEOUT", 9) << fmt::colors::reset
                              << " ";
                    auto p75 = fmt::pad_s("??", 9);
                    std::cout << p75 << " ";
                    std::cout << timeout_histogram_display[0];
                  } else {
                    // Fix alignment in non-color mode too
                    std::cout << fmt::pad_s("TIMEOUT", 9) << " " << fmt::pad_s("??", 9) << " "
                              << std::string(21, '#');
                  }
                  std::cout << std::endl;

                  // Second line for non-compact mode - fix alignment here too
                  std::cout << fmt::pad_e(" ", k_legend - 3);
                  if (opts.colors) {
                    std::cout << fmt::colors::gray << "(??s ... ??s)" << fmt::colors::reset << " ";
                    auto p99 = fmt::pad_s("??", 9);
                    std::cout << p99 << " ";
                    std::cout << timeout_histogram_display[1];
                  } else {
                    std::cout << "(??s ... ??s) " << fmt::pad_s("??", 9) << " "
                              << std::string(21, '#');
                  }
                } else {
                  auto avg = fmt::pad_s(fmt::time(s.avg), 9);
                  auto p75 = fmt::pad_s(fmt::time(s.p75), 9);
                  auto histogram = fmt::histogram(fmt::bins(s, 21, .99), 2, opts.colors);

                  if (!opts.colors)
                    std::cout << avg << "/iter" << " " << p75 << " " << histogram[0];
                  else
                    std::cout << fmt::colors::bold << fmt::colors::yellow << avg
                              << fmt::colors::reset << fmt::colors::bold << "/iter"
                              << fmt::colors::reset << " " << fmt::colors::gray << p75
                              << fmt::colors::reset << " " << histogram[0];

                  if (optimized_out) {
                    if (!opts.colors)
                      std::cout << " " << "!";
                    else
                      std::cout << " " << fmt::colors::red << "!" << fmt::colors::reset;
                  }

                  std::cout << std::endl;
                  auto min = fmt::time(s.min);
                  auto max = fmt::time(s.max);
                  auto p99 = fmt::pad_s(fmt::time(s.p99), 9);
                  auto diff = (2 * 9) - (min.length() + max.length());
                  diff += (min.find("µ") != std::string::npos ? 1 : 0);
                  diff += (max.find("µ") != std::string::npos ? 1 : 0);

                  std::cout << fmt::pad_e(" ", diff + k_legend - 8);
                  if (!opts.colors)
                    std::cout << "(";
                  else
                    std::cout << fmt::colors::gray << "(" << fmt::colors::reset;

                  if (!opts.colors)
                    std::cout << min << " … " << max << ")";
                  else
                    std::cout << fmt::colors::cyan << min << fmt::colors::reset << fmt::colors::gray
                              << " … " << fmt::colors::reset << fmt::colors::magenta << max
                              << fmt::colors::reset << fmt::colors::gray << ")"
                              << fmt::colors::reset;

                  std::cout << " ";
                  if (!opts.colors)
                    std::cout << p99 << " " << histogram[1];
                  else
                    std::cout << fmt::colors::gray << p99 << fmt::colors::reset << " "
                              << histogram[1];
                }
              }

              std::cout << std::endl;
            } else {
              // Run parameterized benchmarks
              for (const auto& [param_name, values] : bench._args) {
                for (const auto& value : values) {
                  std::map<std::string, double> args;
                  args[param_name] = value;
                  std::string formatted_name = bench.get_formatted_name(args);

                  // Reset result for this benchmark run
                  auto& b = const_cast<B&>(bench);
                  b.reset_result();

                  auto param_fn = [&b, args]() {
                    b.fn(args, b._last_result);
                  };
                  auto s = stats[formatted_name] =
                    lib::measure_function(param_fn, opts.timelimit_ns);
                  trials.push_back(
                    std::make_pair(formatted_name, std::make_pair(bench._baseline, s))
                  );

                  // Store the result
                  bench_results[formatted_name] = b._last_result;

                  auto compact = bench._compact;
                  bool optimized_out = s.avg < (1.21 * noop.avg);
                  optimized_out_warning = optimized_out_warning || optimized_out;

                  // Adjusted name display for grouped benchmarks
                  std::string display_name;
                  if (show_header) {
                    // Just show the parameter part if we're in a group
                    display_name = "  " + bench.get_param_display(param_name, value);
                  } else {
                    display_name = formatted_name;
                  }

                  auto fname = fmt::pad_e(fmt::str(display_name, k_legend), k_legend);

                  if (compact) {
                    std::cout << fname << " ";

                    // Handle timeout display
                    if (s.timeout) {
                      if (opts.colors) {
                        std::cout << fmt::colors::red << "TIMEOUT" << fmt::colors::reset;
                        auto p75 = fmt::pad_s("??", 9);
                        auto p99 = fmt::pad_s("??", 9);
                        std::cout << " " << p75 << " " << p99 << " ";
                        // Create the histogram before using it
                        auto timeout_histogram_display = fmt::timeout_histogram(1, opts.colors);
                        std::cout << timeout_histogram_display[0];
                      } else {
                        std::cout << "TIMEOUT ? ?/iter ?? ?? " << std::string(11, '#');
                      }
                    } else {
                      auto avg = fmt::pad_s(fmt::time(s.avg), 9);

                      if (!opts.colors)
                        std::cout << avg << "/iter";
                      else
                        std::cout << fmt::colors::bold << fmt::colors::yellow << avg
                                  << fmt::colors::reset << fmt::colors::bold << "/iter"
                                  << fmt::colors::reset;

                      std::cout << " ";
                      auto p75 = fmt::pad_s(fmt::time(s.p75), 9);
                      auto p99 = fmt::pad_s(fmt::time(s.p99), 9);
                      auto histogram = fmt::histogram(fmt::bins(s, 11, .99), 1, opts.colors);

                      if (!opts.colors)
                        std::cout << p75 << " " << p99 << " " << histogram[0];
                      else
                        std::cout << fmt::colors::gray << p75 << " " << p99 << fmt::colors::reset
                                  << " " << histogram[0];
                      if (optimized_out)
                        std::cout << " " << (!opts.colors ? "" : fmt::colors::red) << "!"
                                  << (!opts.colors ? "" : fmt::colors::reset);
                    }
                  }

                  else {
                    std::cout << fname << " ";

                    // Handle timeout display for non-compact mode
                    if (s.timeout) {
                      auto timeout_histogram_display = fmt::timeout_histogram(2, opts.colors);
                      if (opts.colors) {
                        // Fix alignment by using proper padding
                        std::cout << fmt::colors::red << fmt::pad_s("TIMEOUT", 14)
                                  << fmt::colors::reset << " ";
                        auto p75 = fmt::pad_s("??", 9);
                        std::cout << p75 << " ";
                        std::cout << timeout_histogram_display[0];
                      } else {
                        // Fix alignment in non-color mode too
                        std::cout << fmt::pad_s("TIMEOUT", 9) << " " << fmt::pad_s("??", 9) << " "
                                  << std::string(21, '#');
                      }
                      std::cout << std::endl;

                      // Second line for non-compact mode - fix alignment here too
                      std::cout << fmt::pad_e(" ", k_legend + 2);
                      if (opts.colors) {
                        std::cout << fmt::colors::gray << "(??s ... ??s)" << fmt::colors::reset
                                  << " ";
                        auto p99 = fmt::pad_s("??", 9);
                        std::cout << p99 << " ";
                        std::cout << timeout_histogram_display[1];
                      } else {
                        std::cout << "(??s ... ??s) " << fmt::pad_s("??", 9) << " "
                                  << std::string(21, '#');
                      }
                    } else {
                      auto avg = fmt::pad_s(fmt::time(s.avg), 9);
                      auto p75 = fmt::pad_s(fmt::time(s.p75), 9);
                      auto histogram = fmt::histogram(fmt::bins(s, 21, .99), 2, opts.colors);

                      if (!opts.colors)
                        std::cout << avg << "/iter" << " " << p75 << " " << histogram[0];
                      else
                        std::cout << fmt::colors::bold << fmt::colors::yellow << avg
                                  << fmt::colors::reset << fmt::colors::bold << "/iter"
                                  << fmt::colors::reset << " " << fmt::colors::gray << p75
                                  << fmt::colors::reset << " " << histogram[0];

                      if (optimized_out) {
                        if (!opts.colors)
                          std::cout << " " << "!";
                        else
                          std::cout << " " << fmt::colors::red << "!" << fmt::colors::reset;
                      }

                      std::cout << std::endl;
                      auto min = fmt::time(s.min);
                      auto max = fmt::time(s.max);
                      auto p99 = fmt::pad_s(fmt::time(s.p99), 9);
                      auto diff = (2 * 9) - (min.length() + max.length());
                      diff += (min.find("µ") != std::string::npos ? 1 : 0);
                      diff += (max.find("µ") != std::string::npos ? 1 : 0);

                      std::cout << fmt::pad_e(" ", diff + k_legend - 8);
                      if (!opts.colors)
                        std::cout << "(";
                      else
                        std::cout << fmt::colors::gray << "(" << fmt::colors::reset;

                      if (!opts.colors)
                        std::cout << min << " … " << max << ")";
                      else
                        std::cout << fmt::colors::cyan << min << fmt::colors::reset
                                  << fmt::colors::gray << " … " << fmt::colors::reset
                                  << fmt::colors::magenta << max << fmt::colors::reset
                                  << fmt::colors::gray << ")" << fmt::colors::reset;

                      std::cout << " ";
                      if (!opts.colors)
                        std::cout << p99 << " " << histogram[1];
                      else
                        std::cout << fmt::colors::gray << p99 << fmt::colors::reset << " "
                                  << histogram[1];
                    }
                  }

                  std::cout << std::endl;
                }
              }
            }
          }
        }

        if (collection.types.end() !=
            std::find(collection.types.begin(), collection.types.end(), 'b')) {
          if (1 >= trials.size())
            continue;

          std::cout << std::endl;
          auto map = std::map<std::string, f64>();

          for (const auto& trial : trials) {
            map[trial.first] = trial.second.second.avg;
          }

          std::cout << fmt::barplot(map, k_legend, 44, opts.colors);
        }

        if (collection.types.end() !=
            std::find(collection.types.begin(), collection.types.end(), 'x')) {
          std::cout << std::endl;
          auto map = std::map<std::string, lib::k_stats>();

          for (const auto& trial : trials) {
            map[trial.first] = trial.second.second;
          }

          std::cout << fmt::boxplot(map, k_legend, 44, opts.colors);
        }

        if (collection.types.end() !=
            std::find(collection.types.begin(), collection.types.end(), 'l')) {
          std::cout << std::endl;

          // For benchmarks with args, group by base algorithm name
          std::map<std::string, std::map<double, double>> param_series;

          // Direct approach: Collect all parameterized benchmark data
          for (const auto& [base_name, benchmarks] : grouped_benchmarks) {
            for (const auto& [name, bench_ptr] : benchmarks) {
              const auto& bench = *bench_ptr;

              // Skip benchmarks without parameters
              if (bench._args.empty())
                continue;

              // Process each parameter and its values
              for (const auto& [param_name, values] : bench._args) {
                for (const auto& value : values) {
                  // Create args map exactly as used when running the benchmark
                  std::map<std::string, double> args;
                  args[param_name] = value;

                  // Generate the formatted name exactly as it was stored in stats
                  std::string formatted_name = bench.get_formatted_name(args);

                  // Look for stats with this exact formatted name
                  if (stats.find(formatted_name) != stats.end() && !stats[formatted_name].timeout) {
                    // Add this data point to the series, using base name as the series identifier
                    // Only add if the benchmark didn't timeout
                    param_series[base_name][value] = stats[formatted_name].avg;
                  }
                }
              }
            }
          }

          // Draw the lineplot if we have data
          if (!param_series.empty()) {
            std::cout << fmt::lineplot(param_series, k_legend, 44, 15, opts.colors);
          }
        }

        if (collection.types.end() !=
            std::find(collection.types.begin(), collection.types.end(), 's')) {
          if (1 >= trials.size())
            continue;

          // Group trials by parameter values
          std::map<std::string, std::vector<std::pair<std::string, std::pair<bool, lib::k_stats>>>>
            grouped_trials;

          // Parse parameters from benchmark names and group accordingly
          for (const auto& trial : trials) {
            std::string param_value = "";
            double numeric_value = 0.0;

            // Extract parameter value from name if it exists
            size_t param_start = trial.first.find('(');
            if (param_start != std::string::npos) {
              size_t param_end = trial.first.find(')');
              if (param_end != std::string::npos) {
                param_value = trial.first.substr(param_start, param_end - param_start + 1);

                // Try to extract numeric value for sorting
                size_t equals_pos = trial.first.find('=', param_start);
                if (equals_pos != std::string::npos && equals_pos < param_end) {
                  try {
                    numeric_value =
                      std::stod(trial.first.substr(equals_pos + 1, param_end - equals_pos - 1));
                  } catch (...) {
                    // If conversion fails, keep using string-based grouping
                  }
                }
              }
            }

            grouped_trials[param_value].push_back(trial);
          }

          std::cout << std::endl;
          if (!opts.colors)
            std::cout << "summary" << std::endl;
          else
            std::cout << fmt::colors::bold << "summary" << fmt::colors::reset << std::endl;

          // Create a vector of parameter values for sorted display
          std::vector<std::string> param_order;
          for (const auto& [param_value, _] : grouped_trials) {
            param_order.push_back(param_value);
          }

          // Sort parameter values (empty value first, then numerically where possible)
          std::sort(
            param_order.begin(),
            param_order.end(),
            [](const std::string& a, const std::string& b) {
              if (a.empty())
                return true;
              if (b.empty())
                return false;

              // Try to extract numeric values for comparison
              double a_val = 0.0, b_val = 0.0;
              bool a_is_numeric = false, b_is_numeric = false;

              size_t a_equals_pos = a.find('=');
              size_t b_equals_pos = b.find('=');

              if (a_equals_pos != std::string::npos) {
                try {
                  a_val = std::stod(a.substr(a_equals_pos + 1));
                  a_is_numeric = true;
                } catch (...) {}
              }

              if (b_equals_pos != std::string::npos) {
                try {
                  b_val = std::stod(b.substr(b_equals_pos + 1));
                  b_is_numeric = true;
                } catch (...) {}
              }

              // If both are numeric, compare values
              if (a_is_numeric && b_is_numeric) {
                return a_val < b_val;
              }

              // Otherwise fall back to string comparison
              return a < b;
            }
          );

          // Process each group separately in sorted order
          for (const auto& param_value : param_order) {
            auto& group_trials = grouped_trials[param_value];

            // First, remove any timed-out benchmarks from the group
            auto new_end =
              std::remove_if(group_trials.begin(), group_trials.end(), [](const auto& trial) {
                return trial.second.second.timeout;
              });
            group_trials.erase(new_end, group_trials.end());

            // Skip empty groups (all benchmarks timed out)
            if (group_trials.empty())
              continue;

            if (!param_value.empty()) {
              std::cout << std::endl;
              if (!opts.colors)
                std::cout << "Parameter: " << param_value << std::endl;
              else
                std::cout << fmt::colors::gray << "Parameter: " << param_value << fmt::colors::reset
                          << std::endl;
            }

            // Check if any benchmark in this group has a custom scoring function
            bool has_custom_scoring = false;
            for (const auto& [name, stats_pair] : group_trials) {
              // Find the original benchmark to check if it has a scoring function
              for (const auto& collection : collections) {
                for (const auto& [bench_name, bench] : collection.benchmarks) {
                  if (bench.has_score_function()) {
                    has_custom_scoring = true;
                    break;
                  }
                }
                if (has_custom_scoring)
                  break;
              }
              if (has_custom_scoring)
                break;
            }

            // Sort based on custom scoring or default time-based scoring
            if (has_custom_scoring) {
              std::sort(
                group_trials.begin(),
                group_trials.end(),
                [this](const auto& a, const auto& b) {
                  double score_a = std::numeric_limits<double>::max();
                  double score_b = std::numeric_limits<double>::max();

                  // Find the original benchmark to get its scoring function
                  for (const auto& collection : collections) {
                    for (const auto& [bench_name, bench] : collection.benchmarks) {
                      // For benchmark A
                      if (a.first.find(bench_name) == 0 && bench.has_score_function()) {
                        // Extract args if this is a parameterized benchmark
                        std::map<std::string, double> args_a;
                        for (const auto& [param, values] : bench._args) {
                          size_t param_start = a.first.find('(');
                          if (param_start != std::string::npos) {
                            size_t equals_pos = a.first.find('=', param_start);
                            size_t param_end = a.first.find(')', equals_pos);
                            if (equals_pos != std::string::npos && param_end != std::string::npos) {
                              try {
                                double value = std::stod(
                                  a.first.substr(equals_pos + 1, param_end - equals_pos - 1)
                                );
                                args_a[param] = value;
                              } catch (...) {}
                            }
                          }
                        }
                        score_a = bench.calculate_score(a.second.second, args_a);
                      }

                      // For benchmark B
                      if (b.first.find(bench_name) == 0 && bench.has_score_function()) {
                        // Extract args if this is a parameterized benchmark
                        std::map<std::string, double> args_b;
                        for (const auto& [param, values] : bench._args) {
                          size_t param_start = b.first.find('(');
                          if (param_start != std::string::npos) {
                            size_t equals_pos = b.first.find('=', param_start);
                            size_t param_end = b.first.find(')', equals_pos);
                            if (equals_pos != std::string::npos && param_end != std::string::npos) {
                              try {
                                double value = std::stod(
                                  b.first.substr(equals_pos + 1, param_end - equals_pos - 1)
                                );
                                args_b[param] = value;
                              } catch (...) {}
                            }
                          }
                        }
                        score_b = bench.calculate_score(b.second.second, args_b);
                      }
                    }
                  }

                  // Lower scores are better by default (for generic scores)
                  // Custom scores that prefer higher values should negate in their implementation
                  return score_a < score_b;
                }
              );
            } else {
              // Default sorting by average time
              std::sort(group_trials.begin(), group_trials.end(), [](const auto& a, const auto& b) {
                return a.second.second.avg < b.second.second.avg;
              });
            }

            // Find the baseline within this parameter group
            auto baseline =
              std::find_if(group_trials.begin(), group_trials.end(), [](const auto& trial) {
                return trial.second.first;
              });

            // If no explicit baseline, use the fastest/best benchmark in this group
            if (baseline == group_trials.end())
              baseline = group_trials.begin();

            // Print the fastest/baseline benchmark first with score if available
            if (!opts.colors)
              std::cout << "   " << baseline->first << " (baseline)";
            else
              std::cout << "   " << fmt::colors::bold << fmt::colors::green << baseline->first
                        << fmt::colors::reset << " " << fmt::colors::gray << "(baseline)"
                        << fmt::colors::reset;

            if (has_custom_scoring) {
              double baseline_score = 0.0;
              // Find the benchmark to get its score
              for (const auto& collection : collections) {
                for (const auto& [bench_name, bench] : collection.benchmarks) {
                  // Check if this bench matches our baseline
                  if (bench.has_score_function()) {
                    // Extract args if this is a parameterized benchmark
                    std::map<std::string, double> args;
                    for (const auto& [param, values] : bench._args) {
                      size_t param_start = baseline->first.find('(');
                      if (param_start != std::string::npos) {
                        size_t equals_pos = baseline->first.find('=', param_start);
                        size_t param_end = baseline->first.find(')', equals_pos);
                        if (equals_pos != std::string::npos && param_end != std::string::npos) {
                          try {
                            double value = std::stod(
                              baseline->first.substr(equals_pos + 1, param_end - equals_pos - 1)
                            );
                            args[param] = value;
                          } catch (...) {}
                        }
                      }
                    }
                    baseline_score = bench.calculate_score(baseline->second.second, args);
                    if (!opts.colors)
                      std::cout << " [score: " << baseline_score << "]";
                    else
                      std::cout << " " << fmt::colors::gray << "[score: " << fmt::colors::yellow
                                << baseline_score << fmt::colors::gray << "]" << fmt::colors::reset;
                    break;
                  }
                }
              }
            }
            std::cout << std::endl;

            // Only compare benchmarks within the same parameter group
            for (const auto& trial : group_trials) {
              if (trial.first == baseline->first)
                continue;

              auto c = trial.second.second;
              auto b = baseline->second.second;

              // Calculate comparison metrics
              double current_score = 0.0;
              double baseline_score = 0.0;
              bool has_score = false;

              // Check for custom score
              if (has_custom_scoring) {
                // Find the benchmarks to get scores
                for (const auto& collection : collections) {
                  for (const auto& [bench_name, bench] : collection.benchmarks) {
                    // Check current benchmark
                    auto strip_params = [](const std::string& s) -> std::string {
                      size_t space_pos = s.find("(");
                      return space_pos != std::string::npos ? s.substr(0, space_pos) : s;
                    };
                    if (strip_params(trial.first).find(strip_params(bench_name)) == 0 &&
                        bench.has_score_function()) {
                      // Extract args for current benchmark
                      std::map<std::string, double> args_current;
                      for (const auto& [param, values] : bench._args) {
                        size_t param_start = trial.first.find('(');
                        if (param_start != std::string::npos) {
                          size_t equals_pos = trial.first.find('=', param_start);
                          size_t param_end = trial.first.find(')', equals_pos);
                          if (equals_pos != std::string::npos && param_end != std::string::npos) {
                            try {
                              double value = std::stod(
                                trial.first.substr(equals_pos + 1, param_end - equals_pos - 1)
                              );
                              args_current[param] = value;
                            } catch (...) {}
                          }
                        }
                      }
                      current_score = bench.calculate_score(c, args_current);
                      has_score = true;
                    }

                    // Check baseline benchmark
                    if (baseline->first.find(strip_params(bench_name)) >= 0 &&
                        bench.has_score_function()) {
                      // Extract args for baseline benchmark
                      std::map<std::string, double> args_baseline;
                      for (const auto& [param, values] : bench._args) {
                        size_t param_start = baseline->first.find('(');
                        if (param_start != std::string::npos) {
                          size_t equals_pos = baseline->first.find('=', param_start);
                          size_t param_end = baseline->first.find(')', equals_pos);
                          if (equals_pos != std::string::npos && param_end != std::string::npos) {
                            try {
                              double value = std::stod(
                                baseline->first.substr(equals_pos + 1, param_end - equals_pos - 1)
                              );
                              args_baseline[param] = value;
                            } catch (...) {}
                          }
                        }
                      }
                      baseline_score = bench.calculate_score(b, args_baseline);
                      has_score = true;
                    }
                  }
                }
              }

              // Display either score-based or time-based comparison
              if (has_custom_scoring && has_score) {
                // With custom scoring functions, lower could be better or higher scores could be
                // better Check if lower scores are better or higher scores are better
                bool lower_is_better = false;

                // For TSP, typically lower path scores are better
                // But our scoring functions might negate to make higher scores better
                bool scores_equal = std::abs(current_score - baseline_score) < 1e-9;
                bool better = !scores_equal && (lower_is_better ? (current_score > baseline_score)
                                                                : (current_score < baseline_score));

                // Calculate ratio for display (ensuring it's always > 1.0 for readability)
                double ratio = 1.0;
                if (!scores_equal) {
                  ratio = lower_is_better
                          ? (baseline_score != 0 ? current_score / baseline_score : 0)
                          : (current_score != 0 ? baseline_score / current_score : 0);
                  ratio = std::abs(ratio);  // Ensure positive
                }

                std::cout << "   ";
                if (opts.colors)
                  std::cout
                    << (scores_equal ? fmt::colors::blue
                                     : (better ? fmt::colors::green : fmt::colors::red));

                std::cout << std::fixed << std::setprecision(2) << ratio;
                if (opts.colors)
                  std::cout << fmt::colors::reset;

                std::cout << "x " << (scores_equal ? "equal" : (better ? "better" : "worse"))
                          << " score" << (scores_equal ? " as " : " than ");

                if (opts.colors)
                  std::cout << fmt::colors::bold << fmt::colors::cyan;
                std::cout << trial.first;
                if (opts.colors)
                  std::cout << fmt::colors::reset;

                // Show the actual score value
                if (!opts.colors)
                  std::cout << " [score: " << current_score << "]";
                else
                  std::cout << " " << fmt::colors::gray << "[score: " << fmt::colors::yellow
                            << current_score << fmt::colors::gray << "]" << fmt::colors::reset;

              } else {
                // Traditional time-based comparison
                auto faster = b.avg <= c.avg;

                auto diff = !faster ? std::round(c.avg / b.avg * 100) / 100
                                    : std::round(b.avg / c.avg * 100) / 100;

                std::cout << "   ";
                if (opts.colors)
                  std::cout << (!faster ? fmt::colors::red : fmt::colors::green);

                std::cout << diff;
                if (opts.colors)
                  std::cout << fmt::colors::reset;
                std::cout << "x" << " " << (faster ? "faster" : "slower") << " than ";

                if (opts.colors)
                  std::cout << fmt::colors::bold << fmt::colors::cyan;
                std::cout << trial.first;
                if (opts.colors)
                  std::cout << fmt::colors::reset;
              }
              std::cout << std::endl;
            }
          }
        }
      }

      bool any_timeout = false;
      for (const auto& [name, stat] : stats) {
        if (stat.timeout) {
          any_timeout = true;
          break;
        }
      }

      if (any_timeout) {
        if (!opts.colors)
          std::cout << std::endl
                    << "TIMEOUT = benchmark exceeded the time limit ("
                    << fmt::time(opts.timelimit_ns) << ")" << std::endl;
        else
          std::cout << std::endl
                    << fmt::colors::red << "TIMEOUT" << fmt::colors::reset << " "
                    << fmt::colors::gray << "=" << fmt::colors::reset
                    << " benchmark exceeded the time limit " << fmt::colors::gray << "("
                    << fmt::time(opts.timelimit_ns) << ")" << fmt::colors::reset << std::endl;
      }

      if (optimized_out_warning) {
        if (!opts.colors)
          std::cout << std::endl
                    << "! = benchmark was likely optimized out (dead code elimination)"
                    << std::endl;
        else
          std::cout << std::endl
                    << fmt::colors::red << "!" << fmt::colors::reset << " " << fmt::colors::gray
                    << "=" << fmt::colors::reset << " benchmark was likely optimized out "
                    << fmt::colors::gray << "(dead code elimination)" << fmt::colors::reset
                    << std::endl;
      }
    }

    return stats;
  }

  // Get the results for a specific benchmark
  BenchmarkResult get_result(const std::string& bench_name) const {
    auto it = bench_results.find(bench_name);
    return (it != bench_results.end()) ? it->second : BenchmarkResult{};
  }

  // Get all benchmark results
  const std::map<std::string, BenchmarkResult>& get_all_results() const { return bench_results; }
};
}  // namespace mitata

#endif