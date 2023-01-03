#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include "csv.h"
#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_utils.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"

struct Circle {
	int grid_x;
	int grid_y;
	float cl_x;
	float cl_y;
	float hl_x;
	float hl_y;
	std::string id14;
};

int main() {

	float tube_r = .625 / 2;
	int margin_x = 0;
	int margin_y = 2;

	// Parse the CSV file to extract the data for each circle
	std::vector<Circle> circles;
	io::CSVReader<7, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'>> in(
			"circles.csv");
	in.read_header(io::ignore_extra_column, "grid_x", "grid_y", "cl_x", "cl_y",
			"hl_x", "hl_y", "id14");
	int grid_x, grid_y;
	float cl_x, cl_y, hl_x, hl_y;
	std::string id14;
	while (in.read_row(grid_x, grid_y, cl_x, cl_y, hl_x, hl_y, id14)) {
		circles.push_back( { grid_x, grid_y, cl_x, cl_y, hl_x, hl_y, id14.substr(5) });
	}

	// Search for max x and y distances
	auto max_x_it = std::max_element(circles.begin(), circles.end(),
			[](Circle a, Circle b) {
				return std::abs(a.cl_x) < std::abs(b.cl_x);
			});
	float max_x = (*max_x_it).cl_x;
	std::cout << "absolute max X :" << max_x << '\n';

	auto max_y_it = std::max_element(circles.begin(), circles.end(),
			[](Circle a, Circle b) {
				return std::abs(a.cl_y) < std::abs(b.cl_y);
			});
	float max_y = (*max_y_it).cl_y;
	std::cout << "absolute max Y :" << max_y << '\n';

  // Create the SVG document
  rapidxml::xml_document<> doc;
  auto svg_node = doc.allocate_node(rapidxml::node_element, "svg");

  svg_node->append_attribute(doc.allocate_attribute("xmlns", "http://www.w3.org/2000/svg"));
  svg_node->append_attribute(doc.allocate_attribute("version", "1.1"));
  svg_node->append_attribute(doc.allocate_attribute("width", "2200"));
  svg_node->append_attribute(doc.allocate_attribute("viewBox", doc.allocate_string(("0 0 " + std::to_string(std::ceil(max_x) + (2 * margin_x))
		  + " " + std::to_string(std::ceil(max_y) + (2 * margin_y))).c_str())));

  auto style_node = doc.allocate_node(rapidxml::node_element, "style");
  style_node->append_attribute(doc.allocate_attribute("type", "text/css"));

  style_node->value(".circle { fill: white; stroke: black; stroke-width: 0.02; } "
		  ".text { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: black  }");

  svg_node->append_node(style_node);
  //svg_node->append_node(style)::property_tree::ptree &style = doc.add("svg.style", "");


  doc.append_node(svg_node);

  // Create an SVG circle element for each circle in the CSV data
  for (const auto& circle : circles) {
    auto circle_node = doc.allocate_node(rapidxml::node_element, "circle");
    circle_node->append_attribute(doc.allocate_attribute("cx", doc.allocate_string((std::to_string(circle.hl_x)).c_str())));
    circle_node->append_attribute(doc.allocate_attribute("cy", doc.allocate_string((std::to_string(circle.hl_y)).c_str())));
    circle_node->append_attribute(doc.allocate_attribute("r", doc.allocate_string(std::to_string(tube_r).c_str())));
    circle_node->append_attribute(doc.allocate_attribute("class", "circle"));

    auto text_node = doc.allocate_node(rapidxml::node_element, "title");
    text_node->value(doc.allocate_string(("X=" + std::to_string(circle.grid_x) + " Y=" + std::to_string(circle.grid_y)).c_str()));
    circle_node->append_node(text_node);

    svg_node->append_node(circle_node);

    auto number_node = doc.allocate_node(rapidxml::node_element, "text", doc.allocate_string(circle.id14.c_str()));
	number_node->append_attribute(doc.allocate_attribute("class", "text"));
	number_node->append_attribute(doc.allocate_attribute("x", doc.allocate_string((std::to_string(circle.hl_x)).c_str())));
	number_node->append_attribute(doc.allocate_attribute("y", doc.allocate_string((std::to_string(circle.hl_y)).c_str())));
	//number_node->value();
	auto text_node_clone = doc.allocate_node(rapidxml::node_element, "title");
	doc.clone_node(text_node, text_node_clone);
	number_node->append_node( text_node_clone);
	svg_node->append_node(number_node);

  }

  // Write the SVG document to a file
  std::ofstream file("rapid_circles.svg");
  file << doc;
  file.close();

  return 0;
}
