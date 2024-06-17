#pragma once
#ifdef null
#undef null
#endif

#include "includes/nlohmann_json.hpp"

namespace ncore {
	using json = nlohmann::json;

	/*
void write_json_to_file(const std::string& filename, const ncore::json& j) {
    std::ofstream out(filename);
    out << j.dump(4);
}

struct samlepop {
    int imae;
    int sis;
    float sames;
    std::string nonos;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(samlepop, imae, sis, sames, nonos);
};

void testjson() {
    auto json = ncore::json();

    json.emplace("bib", samlepop{
        12,
        223,
        44.2f,
        (const char*)u8"ssiså÷êè õà÷þ sebe"
    });

    write_json_to_file("output.json", json);

    auto nonos = json.at("bib/nonos", '/').get<std::string>();
}
	*/
}

#define null (0)