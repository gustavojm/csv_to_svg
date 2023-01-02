#include <iostream>
#include <fstream>
#include <vector>
#include "csv.h"
#include "rapidxml.hpp"

struct Circle {
  int cx;
  int cy;
  int r;
};

int main() {
  // Parse the CSV file to extract the data for each circle
  std::vector<Circle> circles;
  io::CSVReader<3> in("circles.csv");
  in.read_header(io::ignore_extra_column, "cx", "cy", "r");
  int cx, cy, r;
  while (in.read_row(cx, cy, r)) {
    circles.push_back({cx, cy, r});
  }

  // Create the SVG document
  rapidxml::xml_document<> doc;
  auto svg_node = doc.allocate_node(rapidxml::node_element, "svg");
  svg_node->append_attribute(doc.allocate_attribute("width", "100%"));
  svg_node->append_attribute(doc.allocate_attribute("height", "100%"));
  doc.append_node(svg_node);

  // Create an SVG circle element for each circle in the CSV data
  for (const auto& circle : circles) {
    auto circle_node = doc.allocate_node(rapidxml::node_element, "circle");
    circle_node->append_attribute(doc.allocate_attribute("cx", std::to_string(circle.cx).c_str()));
    circle_node->append_attribute(doc.allocate_attribute("cy", std::to_string(circle.cy).c_str()));
    circle_node->append_attribute(doc.allocate_attribute("r", std::to_string(circle.r).c_str()));
    svg_node->append_node(circle_node);
  }

  // Write the SVG document to a file
  std::ofstream file("circles.svg");
  file << doc;
  file.close();

  return 0;
}
