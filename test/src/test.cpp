
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <fstream>
#include <ranges>
#include <deque>

#define protected public
#include "io/json/Binder.h"

#define FMT_HEADER_ONLY
#include "graph/container/compressed_graph.hpp"

TEST_SUITE("main"){


TEST_CASE("test path operators") {
    sylvanmats::io::json::Path jp;
    jp["elements"]["*"]["symbol"]=="H";
    std::stringstream ss;
    ss<<jp;
    CHECK_EQ(ss.str(), "/elements/*/symbol == H");
    std::cout<<"jp "<<jp<<std::endl;
}

TEST_CASE("test periodic table json") {
    std::string jsonContent=R"({
    "elements": [
        {
            "name": "Hydrogen",
            "appearance": "colorless gas",
            "atomic_mass": 1.008,
            "boil": 20.271,
            "category": "diatomic nonmetal",
            "color": null,
            "density": 0.08988,
            "discovered_by": "Henry Cavendish",
            "melt": 13.99,
            "molar_heat": 28.836,
            "named_by": "Antoine Lavoisier",
            "number": 1,
            "period": 1,
            "phase": "Gas",
            "source": "https://en.wikipedia.org/wiki/Hydrogen",
            "spectral_img": "https://en.wikipedia.org/wiki/File:Hydrogen_Spectra.jpg",
            "summary": "Hydrogen is a chemical element with chemical symbol H and atomic number 1. With an atomic weight of 1.00794 u, hydrogen is the lightest element on the periodic table. Its monatomic form (H) is the most abundant chemical substance in the Universe, constituting roughly 75% of all baryonic mass.",
            "symbol": "H",
            "xpos": 1,
            "ypos": 1,
            "shells": [
                1
            ],
            "electron_configuration": "1s1",
            "electron_configuration_semantic": "1s1",
            "electron_affinity": 72.769,
            "electronegativity_pauling": 2.2,
            "ionization_energies": [
                1312
            ],
            "cpk-hex": "ffffff"
        }
    ]
})";
    sylvanmats::io::json::Binder jsonBinder;
    jsonBinder(jsonContent);
//    std::string depthText=fmt::format("{}\n", jsonBinder.depthList);
//    std::cout<<depthText;
    sylvanmats::io::json::Path jpName;
    jpName["elements"]["*"]["symbol"]=="H";
    size_t val=0;
    jsonBinder(jpName, [&](std::string_view& key, std::any& v){
//        std::cout<<"key "<<key<<std::endl;
        if(key.compare("number")==0){
                   val=std::any_cast<long>(v);
        }
    });
    CHECK_EQ(val, 1);
}

TEST_CASE("test create json") {
    sylvanmats::io::json::Binder jsonBinder;
    sylvanmats::io::json::Path jp;
    jsonBinder(jp, "8DR", sylvanmats::io::json::object());
    CHECK_EQ(jsonBinder.countObjects(), 2);
//    std::cout<<jsonBinder<<std::endl;
    jp["8DR"];
    jsonBinder(jp, "start", 100);
    CHECK_EQ(jsonBinder.countObjects(), 2);
//    jsonBinder.display();
//    std::cout<<jsonBinder<<std::endl;
//    jsonBinder(jp, "end", 200);
//    std::cout<<jsonBinder<<std::endl;
    sylvanmats::io::json::Path jp2;
    jsonBinder(jp2, "CGU", sylvanmats::io::json::object());
    CHECK_EQ(jsonBinder.countObjects(), 3);
//    jsonBinder.display();
//    std::cout<<jsonBinder<<std::endl;
    jp2["CGU"];
    jsonBinder(jp2, "start", 300);
//    std::cout<<"check: "<<jsonBinder<<std::endl;
    jsonBinder(jp2, "end", 400);
    jsonBinder(jp, "end", 200);
    sylvanmats::io::json::Path jp3;
    jsonBinder(jp3, "000", sylvanmats::io::json::object());
//    std::cout<<jsonBinder<<std::endl;
    jp3["000"];
    jsonBinder(jp3, "start", 1);
//    jsonBinder.display();
//    std::cout<<"check: "<<jsonBinder<<std::endl;
    jsonBinder(jp3, "end", 50);
    sylvanmats::io::json::Path jp4;
    jsonBinder(jp4, "001", sylvanmats::io::json::object());
//    std::cout<<jsonBinder<<std::endl;
    jp4["001"];
    jsonBinder(jp4, "start", 51);
//    jsonBinder.display();
//    std::cout<<"check: "<<jsonBinder<<std::endl;
    jsonBinder(jp4, "end", 99);
////    //jsonBinder.display();
    std::cout<<jsonBinder<<std::endl;
    CHECK_EQ(jsonBinder.countObjects(), 5);
    sylvanmats::io::json::Path jp5=Root();
////    jp3["8DR"];
////    std::cout<<"jp3 "<<jp3<<std::endl;
    //remove 8DR
    jsonBinder(jp5, "8DR");
    std::cout<<jsonBinder<<std::endl;
    CHECK_EQ(jsonBinder.countObjects(), 4);
    sylvanmats::io::json::Path jp6;
    jsonBinder(jp6, "8DR", sylvanmats::io::json::object());
//    std::cout<<jsonBinder<<std::endl;
    jp6["8DR"];
    jsonBinder(jp6, "start", 100u);
//    jsonBinder.display();
    unsigned long p=200u;
    std::any pp=p;
    jsonBinder(jp6, "end", pp);
    std::cout<<std::type_index(pp.type()).name()<<" check: "<<jsonBinder<<std::endl;
}

TEST_CASE("test create series json") {
    sylvanmats::io::json::Binder jsonBinder;
    sylvanmats::io::json::Path jp;
    jsonBinder(jp, "8DR", sylvanmats::io::json::object());
    jp["8DR"];
    size_t count=0;
    jsonBinder(jp, [&jp, &count]()->std::tuple<bool, std::string_view, std::any>{
        if(count==0){
            count++;
            return std::make_tuple(false, "start", 100);
        }
        return std::make_tuple(true, "end", 200);
    });
    std::cout<<jsonBinder<<std::endl;
    sylvanmats::io::json::Path jp2;
    jsonBinder(jp2, "CGU", sylvanmats::io::json::object());
    jp2["CGU"];
    count=0;
    jsonBinder(jp2, [&jp2, &count]()->std::tuple<bool, std::string_view, std::any>{
        if(count==0){
            count++;
            return std::make_tuple(false, "start", 300);
        }
        return std::make_tuple(true, "end", 400);
    });
    std::cout<<jsonBinder<<std::endl;
    CHECK_EQ(jsonBinder.countObjects(), 3);
}

TEST_CASE("test reading package.json") {
    sylvanmats::io::json::Binder jsonBinder;
        std::ifstream is("../package.json");
        std::string jsonContent((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        jsonBinder(jsonContent);
        sylvanmats::io::json::Path jpName;
        jpName["name"];
        std::string_view currentPackageName;
        jsonBinder(jpName, [&currentPackageName](std::any& v){
            currentPackageName=std::any_cast<std::string_view>(v);
        });
        CHECK_EQ(currentPackageName, "json-thresher");
        sylvanmats::io::json::Path type;
        type["devDependencies"];
        size_t count=0;
        jsonBinder(type, [&count](std::string_view& key, std::any& v){
            if(count==0){
                CHECK_EQ(key, "doctest");
                CHECK_EQ(std::any_cast<std::string_view>(v), "onqtam/doctest");
            }
            else if(count==2){
                CHECK_EQ(key, "json-graph-specification");
                CHECK_EQ(std::any_cast<std::string_view>(v), "jsongraph/json-graph-specification");
            }
            count++;
        });
        CHECK_EQ(count, 3);
        
        using G = graph::container::compressed_graph<int, sylvanmats::io::json::jobject>;
        G astGraph;//{{0,1,1}, {1,2,1}};

        std::vector<sylvanmats::io::json::jobject> vertices;
        for(std::vector<std::pair<sylvanmats::io::json::jobject, std::vector<sylvanmats::io::json::jobject>>>::iterator it=jsonBinder.dag.begin();it!=jsonBinder.dag.end();it++){
            vertices.push_back((*it).first);
        }
        std::vector<std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>> edges;
        for(std::vector<std::pair<sylvanmats::io::json::jobject, std::vector<sylvanmats::io::json::jobject>>>::iterator it=jsonBinder.dag.begin();it!=jsonBinder.dag.end();it++){
            for(std::vector<sylvanmats::io::json::jobject>::iterator it2=(*it).second.begin();it2!=(*it).second.end();it2++){
                edges.push_back(std::make_tuple((*it2).obj_size, (*it).first.obj_size, 1));
            }
        }

        using value = std::ranges::range_value_t<decltype(edges)>;
        graph::vertex_id_t<G> N = static_cast<graph::vertex_id_t<G>>(size(graph::vertices(astGraph)));
        using edge_desc  = graph::edge_descriptor<graph::vertex_id_t<G>, true, void, int>;
        //astGraph.reserve(3);
        astGraph.load_edges(edges, [](const value& val) -> edge_desc {
            return edge_desc{std::get<0>(val), std::get<1>(val), std::get<2>(val)};
          }, N);
        astGraph.load_vertices(vertices, [&vertices](sylvanmats::io::json::jobject& nm) {
            auto uid = static_cast<graph::vertex_id_t<G>>(&nm - vertices.data());
            std::cout<<"uid "<<uid<<std::endl;
            return graph::copyable_vertex_t<graph::vertex_id_t<G>, sylvanmats::io::json::jobject>{uid, nm};
  });
        std::cout<<"size: "<<graph::num_vertices(astGraph)<<" "<<graph::vertices(astGraph).size()<<std::endl;
        CHECK_EQ(graph::num_vertices(astGraph), 45);
}

TEST_CASE("test reading mimes db.json") {
    sylvanmats::io::json::Binder jsonBinder;
        std::ifstream is("../cpp_modules/mime-db/db.json");
        std::string jsonContent((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        jsonBinder(jsonContent);
//            std::string depthText=fmt::format("{}\n", jsonBinder.depthList);
//            std::cout<<depthText;
            CHECK_EQ(jsonBinder.dag.size(), 15140);
            CHECK_EQ(jsonBinder.depthList.size(), 15140);
//            std::ofstream ofs("check.txt");
//            ofs<<depthText;
//            ofs.close();
        sylvanmats::io::json::Path jp="*"_jp;
        CHECK_EQ(jp.p.size(), 1);
//        std::cout<<"jp "<<jp<<std::endl;
        std::vector<std::string> mimeNames;
        jsonBinder(jp, [&](std::string_view& key, std::any& v){
//            std::cout<<key<<std::endl;
            mimeNames.push_back(std::string(key.begin(), key.end()));
        });
        for(std::vector<std::string>::iterator it=mimeNames.begin();it!=mimeNames.end();++it){
            std::string jp2Str=(*it);
//            jp2Str.append("/extensions");
            sylvanmats::io::json::Path jp2;
            jp2/jp2Str.c_str()/"extensions";
//            std::cout<<"jp2 "<<jp2<<std::endl;
            jsonBinder(jp2, [&](std::string_view& key, std::any& v){
//                std::cout<<(*it)<<" "<<key<<std::endl;
            });
        }
//        sylvanmats::io::json::Path jp="*/extensions"_jp;
//        CHECK_EQ(jp.p.size(), 2);
//        std::cout<<"jp "<<jp<<std::endl;
//        jsonBinder(jp, [&](std::string_view& key, std::any& v){
//            std::cout<<key<<" "<<std::any_cast<const char*>(v)<<std::endl;
//        });
        is.close();
}

}
