#include <iostream>
#include <fstream>
#include <map>
#include <utility>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include "inc/csv.h"
#include "inc/rapidxml-1.13/rapidxml.hpp"
#include "inc/rapidxml-1.13/rapidxml_utils.hpp"
#include "inc/rapidxml-1.13/rapidxml_print.hpp"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

struct tube {
    std::string x_label;
    std::string y_label;
    float x;
    float y;
};

void append_attributes(rapidxml::xml_document<char> *doc,
        rapidxml::xml_node<char> *node,
        std::vector<std::pair<std::string, std::string>> attrs) {
    if (node) {

        for (auto attr : attrs) {
            node->append_attribute(
                    doc->allocate_attribute(
                            doc->allocate_string(attr.first.c_str()),
                            doc->allocate_string(attr.second.c_str())));
        }
    }
}

void add_dashed_line(rapidxml::xml_document<char> *doc,
        rapidxml::xml_node<char> *parent_node, float x1, float y1, float x2,
        float y2) {
    auto line_node = doc->allocate_node(rapidxml::node_element, "line");
    append_attributes(doc, line_node, { { "x1", std::to_string(x1) }, { "y1",
            std::to_string(y1) }, { "x2", std::to_string(x2) }, { "y2",
            std::to_string(y2) }, { "stroke", "gray" },
            { "stroke-width", "0.02" }, { "stroke-dasharray", "0.2, 0.1" }, });

    parent_node->append_node(line_node);
}

rapidxml::xml_node<char>* add_label(rapidxml::xml_document<char> *doc,
        float x, float y,
        const char *label) {
    auto label_node = doc->allocate_node(rapidxml::node_element, "text",
            doc->allocate_string(label));

    append_attributes(doc, label_node, { { "x", std::to_string(x) }, { "y",
            std::to_string(y) }, { "class", "label" }, });

    return label_node;
}

rapidxml::xml_node<char>* add_tube(rapidxml::xml_document<char> *doc,
        float x, float y,
        float radius, std::string id, const std::string &x_label,
        const std::string &y_label) {
    auto tube_group_node = doc->allocate_node(rapidxml::node_element, "g");
    append_attributes(doc, tube_group_node,
            { { "id", doc->allocate_string(id.c_str()) },
                    { "data-col", x_label }, { "data-row", y_label }

            });

    auto tube_node = doc->allocate_node(rapidxml::node_element, "circle");
    append_attributes(doc, tube_node, { { "cx", std::to_string(x) }, { "cy",
            std::to_string(y) }, { "r", std::to_string(radius) }, { "class",
            "tube" }, });

    auto tooltip_node = doc->allocate_node(rapidxml::node_element, "title");
    tooltip_node->value(
            doc->allocate_string(
                    (std::string("Col=") + x_label + " Row=" + y_label).c_str()));
    tube_group_node->append_node(tooltip_node);
    tube_group_node->append_node(tube_node);
    auto number_node = doc->allocate_node(rapidxml::node_element, "text",
            doc->allocate_string(id.substr(2).c_str()));
    //number_node->value();
    append_attributes(doc, number_node, { { "class", "tube_num" }, { "x",
            std::to_string(x) }, { "y", std::to_string(y) }, {
            "transform-origin", std::to_string(x) + " " + std::to_string(y) }, {
            "transform", "scale(1,-1)" }, });

    tube_group_node->append_node(number_node);
    return tube_group_node;

}

int main(int argc, char *argv[]) {

    namespace po = boost::program_options;
    std::string leg = "both";
    float tube_od = 1.f;
    float min_x, width;
    float min_y, height;
    std::string font_size;
    std::string x_labels_param;
    std::string y_labels_param;
    std::vector<std::string> x_labels_coords;
    std::vector<std::string> y_labels_coords;

    try {
        po::options_description settings_desc("HX Settings");
        settings_desc.add_options()("leg",
                po::value<std::string>(&leg)->default_value("both"),
                "Leg (hot, cold, both)");
        settings_desc.add_options()("tube_od",
                po::value<float>(&tube_od)->default_value(1.f),
                "Tube Outside Diameter in inches");
        settings_desc.add_options()("min_x",
                po::value<float>(&min_x)->default_value(0),
                "viewBox X minimum coord");
        settings_desc.add_options()("width",
                po::value<float>(&width)->default_value(0), "viewBoxWidth");
        settings_desc.add_options()("min_y",
                po::value<float>(&min_y)->default_value(0),
                "viewBox Y minimum coord");
        settings_desc.add_options()("height",
                po::value<float>(&height)->default_value(0), "viewBox Height");
        settings_desc.add_options()("font_size",
                po::value<std::string>(&font_size)->default_value("0.25"),
                "Number font size in px");
        settings_desc.add_options()("x_labels",
                po::value<std::string>(&x_labels_param)->default_value("0"),
                "Where to locate x axis labels, can use several coords separated by space");
        settings_desc.add_options()("y_labels",
                po::value<std::string>(&y_labels_param)->default_value("0"),
                "Where to locate x axis labels, can use several coords separated by space");
        settings_desc.add_options()("help,h", "Help screen"); // what an strange syntax...
        // ("config", po::value<std::string>(), "Config file");
        po::variables_map vm;

        po::store(po::parse_command_line(argc, argv, settings_desc), vm);
        std::filesystem::path config = std::filesystem::path("config.ini");
        if (std::filesystem::exists(config)) {
            std::ifstream config_is = std::ifstream(config);
            po::store(po::parse_config_file(config_is, settings_desc, true),
                    vm);
        }
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << settings_desc << '\n';
        }

        boost::split(x_labels_coords, x_labels_param, boost::is_any_of(" "));
        boost::split(y_labels_coords, y_labels_param, boost::is_any_of(" "));

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    float tube_r = tube_od / 2;

    // Parse the CSV file to extract the data for each tube
    std::map<std::string, tube> tubes;
    std::set<std::pair<std::string, float>> x_labels;
    std::set<std::pair<std::string, float>> y_labels;
    io::CSVReader<7, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'>> in(
            "tubesheet.csv");
    in.read_header(io::ignore_extra_column, "x_label", "y_label", "cl_x",
            "cl_y", "hl_x", "hl_y", "tube_id");
    std::string x_label, y_label;
    float cl_x, cl_y, hl_x, hl_y;
    std::string tube_id;
    while (in.read_row(x_label, y_label, cl_x, cl_y, hl_x, hl_y, tube_id)) {
        if (leg == "cold" || leg == "both") {
            tubes.insert( { std::string(std::string("cl") + tube_id.substr(5)),
                    { x_label, y_label, cl_x, cl_y } });
            x_labels.insert(std::make_pair(x_label, cl_x));
            y_labels.insert(std::make_pair(y_label, cl_y));
        }

        if (leg == "hot" || leg == "both") {
            tubes.insert( { std::string(std::string("hl") + tube_id.substr(5)),
                    { x_label, y_label, hl_x, hl_y } });
            x_labels.insert(std::make_pair(x_label, hl_x));
            y_labels.insert(std::make_pair(y_label, hl_y));
        }
    }

    // Create the SVG document
    rapidxml::xml_document<char> document;
    rapidxml::xml_document<char> *doc = &document;
    auto svg_node = doc->allocate_node(rapidxml::node_element, "svg");

    append_attributes(doc, svg_node,
            { { "xmlns", "http://www.w3.org/2000/svg" }, { "version", "1.1" }, {
                    "id", "tubesheet_svg" }, { "viewBox", std::to_string(min_x)
                    + " " + std::to_string(min_y) + " " + std::to_string(width)
                    + " " + std::to_string(height) }, });

    auto style_node = doc->allocate_node(rapidxml::node_element, "style");
    append_attributes(doc, style_node, { { "type", "text/css" } });

    std::string style =
            std::string(
                    ".tube {stroke: black; stroke-width: 0.02; fill: white;} ")
                    + ".tube_num { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: "
                    + font_size
                    + "px; fill: black;}"
                            ".label { text-anchor: middle; alignment-baseline: middle; font-family: sans-serif; font-size: 0.25px; fill: red;}";

    style_node->value(style.c_str());

    svg_node->append_node(style_node);
    doc->append_node(svg_node);

    auto cartesian_g_node = doc->allocate_node(rapidxml::node_element, "g");
    append_attributes(doc, cartesian_g_node, { { "id", "cartesian" }, {
            "transform", "scale(1,-1)" } });
    svg_node->append_node(cartesian_g_node);

    add_dashed_line(doc, cartesian_g_node, min_x, 0, min_x + width, 0);
    add_dashed_line(doc, cartesian_g_node, 0, min_y, 0, min_y + height);

    for (auto coord_y : x_labels_coords) {
        for (auto [label, coord] : x_labels) {
            auto label_x = add_label(doc, coord,
                    std::stof(coord_y), label.c_str());
            append_attributes(doc, label_x,
                      {
                        { "transform-origin", std::to_string(coord) + " " + coord_y },
                        { "transform", "scale(1, -1) rotate(270)" },

                    });
            cartesian_g_node->append_node(label_x);
        }
    }

    for (auto coord_x : y_labels_coords) {
        for (auto [label, coord] : y_labels) {
            auto label_y = add_label(doc, std::stof(coord_x),
                    coord, label.c_str());
            append_attributes(doc, label_y,
                    { { "transform-origin", coord_x + " " + std::to_string(coord) },
                      { "transform", "scale(1, -1)" }, });
           cartesian_g_node->append_node(label_y);
        }
    }

    // Create an SVG circle element for each tube in the CSV data
    for (const auto &tube_pair : tubes) {
        auto tube = tube_pair.second;

        auto tube_node = add_tube(doc, tube.x, tube.y, tube_r,
                tube_pair.first, tube.x_label, tube.y_label);
        cartesian_g_node->append_node(tube_node);

    }

    // Write the SVG document to a file
    std::ofstream file("tubesheet.svg");
    file << document;
    file.close();

    return 0;
}
