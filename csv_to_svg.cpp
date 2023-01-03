#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <utility>
#include <cmath>
#include "csv.h"
#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_utils.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"

struct tube {
	int grid_x;
	int grid_y;
	float cl_x;
	float cl_y;
	float hl_x;
	float hl_y;
};

struct plan_entry {
	int row;
	int col;
	std::string tube;
};

int main() {

	float tube_r = .625 / 2;
	int margin_x = 0;
	int margin_y = 2;

	// Parse the CSV file to extract the data for each tube
	std::map<int, tube> tubes;
	io::CSVReader<7, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'>> in(
			"circles.csv");
	in.read_header(io::ignore_extra_column, "grid_x", "grid_y", "cl_x", "cl_y",
			"hl_x", "hl_y", "id14");
	int grid_x, grid_y;
	float cl_x, cl_y, hl_x, hl_y;
	std::string id14;
	while (in.read_row(grid_x, grid_y, cl_x, cl_y, hl_x, hl_y, id14)) {
		tubes.insert( {std::stoi(id14.substr(5)), {grid_x, grid_y, cl_x, cl_y, hl_x, hl_y}});
	}

	// Search for max x and y distances
	auto max_x_it = std::max_element(tubes.begin(), tubes.end(),
			[](auto a, auto b) {
				return std::abs(a.second.cl_x) < std::abs(b.second.cl_x);
			});
	float max_x = (*max_x_it).second.cl_x;
	std::cout << "absolute max X :" << max_x << '\n';

	auto max_y_it = std::max_element(tubes.begin(), tubes.end(),
			[](auto a, auto b) {
				return std::abs(a.second.cl_y) < std::abs(b.second.cl_y);
			});
	float max_y = (*max_y_it).second.cl_y;
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

  style_node->value(".tube { fill: white; stroke: black; stroke-width: 0.02; } "
		  ".tube_num { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: black  }");

  svg_node->append_node(style_node);

  doc.append_node(svg_node);

  // Create an SVG circle element for each tube in the CSV data
  for (const auto& tube_pair : tubes) {
	  auto tube = tube_pair.second;
    auto tube_node = doc.allocate_node(rapidxml::node_element, "circle");
    tube_node->append_attribute(doc.allocate_attribute("cx", doc.allocate_string((std::to_string(tube.hl_x)).c_str())));
    tube_node->append_attribute(doc.allocate_attribute("cy", doc.allocate_string((std::to_string(tube.hl_y)).c_str())));
    tube_node->append_attribute(doc.allocate_attribute("r", doc.allocate_string(std::to_string(tube_r).c_str())));
    tube_node->append_attribute(doc.allocate_attribute("class", "tube"));

    auto text_node = doc.allocate_node(rapidxml::node_element, "title");
    text_node->value(doc.allocate_string(("X=" + std::to_string(tube.grid_x) + " Y=" + std::to_string(tube.grid_y)).c_str()));
    tube_node->append_node(text_node);

    svg_node->append_node(tube_node);

    auto number_node = doc.allocate_node(rapidxml::node_element, "text", doc.allocate_string(std::to_string(tube_pair.first).c_str()));
    //number_node->value();
    number_node->append_attribute(doc.allocate_attribute("class", "tube_num"));
	number_node->append_attribute(doc.allocate_attribute("x", doc.allocate_string((std::to_string(tube.hl_x)).c_str())));
	number_node->append_attribute(doc.allocate_attribute("y", doc.allocate_string((std::to_string(tube.hl_y)).c_str())));

	auto text_node_clone = doc.allocate_node(rapidxml::node_element);
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

