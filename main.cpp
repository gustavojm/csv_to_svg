#include <iostream>
#include <fstream>
#include <map>
#include <utility>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include "csv.h"
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

struct tube {
	std::string x_label;
	std::string y_label;
	float cl_x;
	float cl_y;
	float hl_x;
	float hl_y;
};

void append_attributes(rapidxml::xml_document<char> &doc, rapidxml::xml_node<char> *node, std::map<std::string, std::string> attrs) {
	if (node) {

		for (auto attr : attrs) {
			node->append_attribute(
							doc.allocate_attribute(doc.allocate_string(attr.first.c_str()),
									doc.allocate_string(attr.second.c_str())));
		}
	}
}

void add_dashed_line(rapidxml::xml_node<char> *parent_node, float x1, float y1, float x2, float y2) {
		auto doc = parent_node->document();
		auto line_node = doc->allocate_node(rapidxml::node_element, "line");
		append_attributes(*doc, line_node, {
				{"x1", std::to_string(x1)},
				{"y1", std::to_string(y1)},
				{"x2", std::to_string(x2)},
				{"y2", std::to_string(y2)},
				{"stroke", "gray"},
				{"stroke-width", "0.02"},
				{"stroke-dasharray", "0.2, 0.1"},
		});

		parent_node->append_node(line_node);
}

rapidxml::xml_node<char>* add_label(rapidxml::xml_node<char> *parent_node, float x, float y, const char* label) {
	auto doc = parent_node->document();
	auto label_node = doc->allocate_node(rapidxml::node_element, "text",
			doc->allocate_string(label));

	append_attributes(*doc, label_node, {
			{"x", std::to_string(x)},
			{"y", std::to_string(y)},
			{"class", "label"},
	});

	return label_node;
}

std::string read_tube_specs(std::string par) {

	io::CSVReader<3, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'>> in(
				"tube_specs.csv");
		in.read_header(io::ignore_extra_column, "Dato", "Valor", "Unidad");
		std::string dato, valor, unidad;
		while (in.read_row(dato, valor, unidad)) {
			std::transform(dato.begin(), dato.end(),dato.begin(), ::toupper);
			if (dato == par) {
				return valor;
			}
		}
		return("");
}

int main() {
	float tube_od = std::stof(read_tube_specs("TUBE_OD"));
	float tube_r = tube_od / 2;
	int margin_x = 1;
	int margin_y = 1;



	// Parse the CSV file to extract the data for each tube
	std::map<int, tube> tubes;
	std::map<std::string, float> x_labels;
	std::map<std::string, float> y_labels;
	io::CSVReader<7, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'>> in(
			"tubesheet.csv");
	in.read_header(io::ignore_extra_column, "x_label", "y_label", "cl_x",
			"cl_y", "hl_x", "hl_y", "tube_id");
	std::string x_label, y_label;
	float cl_x, cl_y, hl_x, hl_y;
	std::string tube_id;
	while (in.read_row(x_label, y_label, cl_x, cl_y, hl_x, hl_y, tube_id)) {
		tubes.insert( { std::stoi(tube_id.substr(5)), { x_label, y_label, cl_x,
				cl_y, hl_x, hl_y } });

		x_labels[x_label] = hl_x;
		y_labels[y_label] = hl_y;

	}

	// Search for max x and y distances
	auto max_x_it = std::max_element(tubes.begin(), tubes.end(),
			[](auto a, auto b) {
				return std::abs(a.second.hl_x) < std::abs(b.second.hl_x);
			});
	float max_x = (*max_x_it).second.hl_x;
	std::cout << "absolute max X :" << max_x << '\n';

	auto max_y_it = std::max_element(tubes.begin(), tubes.end(),
			[](auto a, auto b) {
				return std::abs(a.second.hl_y) < std::abs(b.second.hl_y);
			});
	float max_y = (*max_y_it).second.hl_y;
	std::cout << "absolute max Y :" << max_y << '\n';

	// Create the SVG document
	rapidxml::xml_document<char> doc;
	auto svg_node = doc.allocate_node(rapidxml::node_element, "svg");

	append_attributes(doc, svg_node, {
	{"xmlns", "http://www.w3.org/2000/svg"},
	{"version", "1.1"},
	{"id", "tubesheet_svg"},
	{"height", "auto"},
	{"width", "auto"},
	{"viewBox", "-" + std::to_string(margin_x) + " -" + std::to_string(margin_y) + " " + std::to_string(std::ceil(max_x) + margin_x) + " " + std::to_string(std::ceil(max_y) + margin_y)}});

	auto style_node = doc.allocate_node(rapidxml::node_element, "style");
	append_attributes(doc, style_node, {{"type", "text/css"}});

	style_node->value(
			".tube {stroke: black; stroke-width: 0.02; fill: none;} "
					".tube_num { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: black;}"
					".label { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: red;}");

	svg_node->append_node(style_node);
	doc.append_node(svg_node);

	add_dashed_line(svg_node, -margin_x, 0, std::ceil(max_x) + margin_x, 0);
	add_dashed_line(svg_node, 0, -margin_y, 0, std::ceil(max_y) + margin_y);

	for (auto [label, coord] : x_labels) {
		std::cout << "labels coord X: " << label << " : " << coord << "\n";

		auto label_x = add_label(svg_node, coord, -margin_y * 0.75, label.c_str());
		append_attributes(doc, label_x, {
				{"transform", "rotate(270," + std::to_string(coord) + ", -" + std::to_string(margin_y * 0.75) + ")"},
		});
		svg_node->append_node(label_x);
	}

	for (auto [label, coord] : y_labels) {
		std::cout << "labels coord Y: " << label << " : " << coord << "\n";

		auto label_y = add_label(svg_node, -margin_x * 0.75, coord, label.c_str());
		svg_node->append_node(label_y);
	}

	// Create an SVG circle element for each tube in the CSV data
	for (const auto &tube_pair : tubes) {
		auto tube = tube_pair.second;
		auto tube_group_node = doc.allocate_node(rapidxml::node_element, "g");
		append_attributes(doc, tube_group_node, {
				{"id", std::to_string(tube_pair.first)},
				{"data-col", tube.x_label},
				{"data-row", tube.y_label}

		});

		auto tube_node = doc.allocate_node(rapidxml::node_element, "circle");
		append_attributes(doc, tube_node, {
				{"cx", std::to_string(tube.hl_x)},
				{"cy", std::to_string(tube.hl_y)},
				{"r", std::to_string(tube_r)},
				{"class", "tube"},
		});

		auto tooltip_node = doc.allocate_node(rapidxml::node_element, "title");
		tooltip_node->value(
				doc.allocate_string(
						("Col=" + tube.x_label + " Row=" + tube.y_label).c_str()));
		tube_group_node->append_node(tooltip_node);
		tube_group_node->append_node(tube_node);
		auto number_node = doc.allocate_node(rapidxml::node_element, "text",
				doc.allocate_string(std::to_string(tube_pair.first).c_str()));
		//number_node->value();
		append_attributes(doc, number_node, {
				{"class", "tube_num"},
				{"x", std::to_string(tube.hl_x)},
				{"y", std::to_string(tube.hl_y)},
		});

		tube_group_node->append_node(number_node);

		svg_node->append_node(tube_group_node);

	}

	// Write the SVG document to a file
	std::ofstream file("tubesheet.svg");
	file << doc;
	file.close();

	return 0;
}
