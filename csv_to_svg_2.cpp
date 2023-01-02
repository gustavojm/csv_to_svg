#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "csv.h"

struct Circle {
	float cx;
	float cy;
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
		circles.push_back( { cl_x, cl_y });
	}

	// Search for max x and y distances
	auto max_x_it = std::max_element(circles.begin(), circles.end(),
			[](Circle a, Circle b) {
				return std::abs(a.cx) < std::abs(b.cx);
			});
	float max_x = (*max_x_it).cx;
	std::cout << "absolute max X :" << max_x << '\n';

	auto max_y_it = std::max_element(circles.begin(), circles.end(),
			[](Circle a, Circle b) {
				return std::abs(a.cy) < std::abs(b.cy);
			});
	float max_y = (*max_y_it).cy;
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
			".circle { fill: none; stroke: black; stroke-width: 0.01; }");

	doc.put("svg.<xmlattr>.width", "100%");
	doc.put("svg.<xmlattr>.height", "100%");
	doc.put("svg.<xmlattr>.viewBox", "0 0 " + std::to_string(std::ceil(max_x)) +  " " + std::to_string(std::ceil(max_y)));

	// Create an SVG circle element for each circle in the CSV data
	for (const auto &circle : circles) {
		boost::property_tree::ptree circle_node;
		circle_node.put("<xmlattr>.cx", circle.cx + margin_x);
		circle_node.put("<xmlattr>.cy", circle.cy + margin_y);
		circle_node.put("<xmlattr>.r", tube_od);
		circle_node.put("<xmlattr>.class", "circle");

		boost::property_tree::ptree tooltip = circle_node.put("title", "Titulo");


		boost::property_tree::ptree circle_number;
		circle_number.put("", "1");
		circle_number.put("<xmlattr>.text_anchor", "middle");
		circle_number.put("<xmlattr>.x", "50%");
		circle_number.put("<xmlattr>.y", "50%");
		circle_number.put("<xmlattr>.dy", ".35em");
		circle_number.put("<xmlattr>.font-family", "sans-serif");
		circle_number.put("<xmlattr>.font-size", "90px");
		circle_number.put("<xmlattr>.fill", "white");

		circle_node.add_child("text", circle_number);

//		boost::property_tree::ptree title_node;
//		title_node.put("<xmlattr>", "soy un circulo");

//		circle_node.add_child("title", title_node);

		doc.add_child("svg.circle", circle_node);
	}

	// Write the SVG document to a file
	std::ofstream file("circles.svg");
	boost::property_tree::xml_parser::write_xml_element(file, std::string(),
			doc, -1, boost::property_tree::xml_parser::no_comments);
	file.close();

	return 0;
}
