#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "csv.h"

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

	float tube_od = .625 / 2;
	int margin_x = 2;
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
	boost::property_tree::ptree doc;

	// Add the SVG namespace declaration
	doc.put("svg.<xmlattr>.xmlns", "http://www.w3.org/2000/svg");
	doc.put("svg.<xmlattr>.version", "1.1");

	// Add a style element to the property tree
	boost::property_tree::ptree &style = doc.add("svg.style", "");
	style.put("<xmlattr>.type", "text/css");
	style.put_value(
			".circle { fill: white; stroke: black; stroke-width: 0.02; } "
			".text { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: black  }");

	doc.put("svg.<xmlattr>.width", "6000");
	//doc.put("svg.<xmlattr>.height", "100%");
	doc.put("svg.<xmlattr>.viewBox",
			"0 0 " + std::to_string(std::ceil(max_x) + margin_x) + " "
					+ std::to_string(std::ceil(max_y) + margin_y));

	// Create an SVG circle element for each circle in the CSV data
	for (const auto &circle : circles) {
		boost::property_tree::ptree circle_node;
		circle_node.put("<xmlattr>.cx", circle.hl_x + margin_x);
		circle_node.put("<xmlattr>.cy", circle.hl_y + margin_y);
		circle_node.put("<xmlattr>.r", tube_od);
		circle_node.put("<xmlattr>.class", "circle");

		boost::property_tree::ptree tooltip = circle_node.put("title",
				"X=" + std::to_string(circle.grid_x) + " Y=" + std::to_string(circle.grid_y) );

		boost::property_tree::ptree circle_number;
		circle_number.put("", circle.id14);
		circle_number.put("<xmlattr>.class", "text");
		circle_number.put("<xmlattr>.x", circle.hl_x + margin_x);
		circle_number.put("<xmlattr>.y", circle.hl_y + margin_y);

//		circle_node.add_child("text", circle_number);			// As a child this text is not shown
//		circle_node.add_child("title", title_node);

		doc.add_child("svg.circle", circle_node);
		doc.add_child("svg.text", circle_number);
	}

	// Write the SVG document to a file
	std::fstream svg_file("circles.svg");
	boost::property_tree::xml_parser::write_xml_element(svg_file, std::string(),
			doc, -1, boost::property_tree::xml_parser::no_comments);

	svg_file.seekg(0);
	// Open the source file in input mode and the destination file in output mode
	//std::ifstream src("circles.svg", std::ios::binary);
	std::ofstream html_file("svg.html", std::ios::binary);

	if (html_file.is_open()) {
		// Write the header to the destination file
		html_file << "<html>\n";
		html_file << "\t<body>\n";

		// Read the contents of the source file into a buffer

		while (!svg_file.eof()) {
			char buffer[1024];
			svg_file.read(buffer, 1024);

			// Append the contents of the buffer to the destination file
			html_file.write(buffer, svg_file.gcount());
		}

		// Write the trailer to the destination file
		html_file << "\n\t</body>";
		html_file << "\n</html>";

	} else {
		std::cout << "Error opening the files!" << std::endl;
	}

	// Close the files
	svg_file.close();
	html_file.close();

	return 0;
}
