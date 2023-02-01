#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <array>
#include <utility>
#include <filesystem>
#include <algorithm>
#include <ctime>
#include <random>
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
	std::string color = "white";
	std::string insp_plan = "";
};

void read_inspection_plans(std::map<int, tube> &tubes) {

	std::array<std::string, 9> pastel_colors = { "#ffadad", "#ffd6a5",
			"#fdffb6", "#caffbf", "#9bf6ff", "#a0c4ff", "#bdb2ff", "#ffc6ff",
			"#ffffbb" };
	int color = 0;

	std::string path = "./insp_plan";
	for (const auto &entry : std::filesystem::directory_iterator(path)) {
		std::cout << "Procesando plan: " << entry.path().filename() << "\n";

		if (!(entry.path().filename().extension() == ".csv")) {
			continue;
		}

		// Parse the CSV file to extract the data for the plan
		io::CSVReader<3, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'> > ip(
				entry.path());
		ip.read_header(io::ignore_extra_column, "ROW", "COL", "TUBE");
		std::string row, col;
		std::string tube_num;

		while (ip.read_row(row, col, tube_num)) {
			if (auto pos = tubes.find(std::stoi(tube_num.substr(5))); pos
					!= tubes.end()) {
				(*pos).second.color = pastel_colors[color];
				(*pos).second.insp_plan = entry.path().filename();
			}
		}
		color++;
		color %= pastel_colors.size();
	}
}

int main() {
	float tube_r = .625 / 2;
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

//	sort_labels(x_labels);
//	sort_labels(y_labels);

// Parse the CSV file to extract the data for the plan
	read_inspection_plans(tubes);
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
	rapidxml::xml_document<> doc;
	auto svg_node = doc.allocate_node(rapidxml::node_element, "svg");

	svg_node->append_attribute(
			doc.allocate_attribute("xmlns", "http://www.w3.org/2000/svg"));
	svg_node->append_attribute(doc.allocate_attribute("version", "1.1"));
	svg_node->append_attribute(doc.allocate_attribute("id", "tubesheet_svg"));
	svg_node->append_attribute(doc.allocate_attribute("height", "auto"));
	svg_node->append_attribute(doc.allocate_attribute("width", "auto"));
	svg_node->append_attribute(
			doc.allocate_attribute("viewBox",
					doc.allocate_string(
							("-" + std::to_string(margin_x) + " -" + std::to_string(margin_y) + " "
									+ std::to_string(
											std::ceil(max_x) + margin_x)
									+ " "
									+ std::to_string(
											std::ceil(max_y) + margin_y)).c_str())));

	auto style_node = doc.allocate_node(rapidxml::node_element, "style");
	style_node->append_attribute(doc.allocate_attribute("type", "text/css"));

	style_node->value(
			".tube {stroke: black; stroke-width: 0.02; } "
					".tube_num { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: black  }"
					".label { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: red;}");

	svg_node->append_node(style_node);

	doc.append_node(svg_node);

	for (auto [label, coord] : x_labels) {
		std::cout << "labels coord X: " << label << " : " << coord << "\n";

		auto label_node = doc.allocate_node(rapidxml::node_element, "text",
				doc.allocate_string(label.c_str()));
		label_node->append_attribute(
				doc.allocate_attribute("x",
						doc.allocate_string(
								(std::to_string(coord)).c_str())));
		label_node->append_attribute(
				doc.allocate_attribute("y", doc.allocate_string(("-" + std::to_string(margin_y * 0.75)).c_str())));
		label_node->append_attribute(
				doc.allocate_attribute("class", "label"));

		label_node->append_attribute(
				doc.allocate_attribute("transform", doc.allocate_string(("rotate(270,"+ std::to_string(coord) + ", -" + std::to_string(margin_y * 0.75) + ")").c_str())));

		svg_node->append_node(label_node);
	}

	for (auto [label, coord] : y_labels) {
		std::cout << "labels coord X: " << label << " : " << coord << "\n";

		auto label_node = doc.allocate_node(rapidxml::node_element, "text",
				doc.allocate_string(label.c_str()));
		label_node->append_attribute(
				doc.allocate_attribute("x",
						doc.allocate_string(
								("-" +  std::to_string(margin_x * 0.75)).c_str())));
		label_node->append_attribute(
				doc.allocate_attribute("y",
						doc.allocate_string(
								(std::to_string(coord)).c_str())));
		label_node->append_attribute(doc.allocate_attribute("class", "label"));

		svg_node->append_node(label_node);
	}

	// Create an SVG circle element for each tube in the CSV data
	for (const auto &tube_pair : tubes) {
		auto tube = tube_pair.second;
		auto tube_group_node = doc.allocate_node(rapidxml::node_element, "g");
		auto tube_node = doc.allocate_node(rapidxml::node_element, "circle");
		tube_node->append_attribute(
				doc.allocate_attribute("cx",
						doc.allocate_string(
								(std::to_string(tube.hl_x)).c_str())));
		tube_node->append_attribute(
				doc.allocate_attribute("cy",
						doc.allocate_string(
								(std::to_string(tube.hl_y)).c_str())));
		tube_node->append_attribute(
				doc.allocate_attribute("r",
						doc.allocate_string(std::to_string(tube_r).c_str())));
		tube_node->append_attribute(
				doc.allocate_attribute("fill",
						doc.allocate_string(tube.color.c_str())));
		tube_node->append_attribute(doc.allocate_attribute("class", "tube"));

		auto tooltip_node = doc.allocate_node(rapidxml::node_element, "title");
		tooltip_node->value(
				doc.allocate_string(
						("X=" + tube.x_label + " Y=" + tube.y_label + " "
								+ tube.insp_plan).c_str()));
		tube_group_node->append_node(tooltip_node);
		tube_group_node->append_node(tube_node);
		auto number_node = doc.allocate_node(rapidxml::node_element, "text",
				doc.allocate_string(std::to_string(tube_pair.first).c_str()));
		//number_node->value();
		number_node->append_attribute(
				doc.allocate_attribute("class", "tube_num"));
		number_node->append_attribute(
				doc.allocate_attribute("x",
						doc.allocate_string(
								(std::to_string(tube.hl_x)).c_str())));
		number_node->append_attribute(
				doc.allocate_attribute("y",
						doc.allocate_string(
								(std::to_string(tube.hl_y)).c_str())));

		tube_group_node->append_node(number_node);

		svg_node->append_node(tube_group_node);

	}

	// Write the SVG document to a file
	std::ofstream file("tubesheet.svg");
	file << doc;
	file.close();

	return 0;
}

