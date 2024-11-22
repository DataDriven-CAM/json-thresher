
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <ranges>
#include <deque>

#define protected public
#include "io/json/Binder.h"
#include "io/tikz/GraphPublisher.h"

#define FMT_HEADER_ONLY
#include "graph/container/compressed_graph.hpp"
#include "graph/views/incidence.hpp"
#include "graph/views/vertexlist.hpp"

enum struct directedness : int8_t {
  directed,   // a single edge joins 2 vertices
  directed2,  // 2 edges join 2 vertices, each with different directions; needed for graphviz
  undirected, // one or more edges exist between vertices with no direction
  bidirected  // a single edge between vertices with direction both ways (similar to undirected, but with arrows)
};

TEST_SUITE("main"){


TEST_CASE("test path operators") {
    sylvanmats::io::json::Path jp;
    jp["elements"]["*"]["symbol"]=="H";
    std::stringstream ss;
    ss<<jp;
    CHECK_EQ(ss.str(), "/elements/*/symbol == H");
    std::cout<<"jp "<<jp<<std::endl;
}

TEST_CASE("test graph-v2"){
    std::vector<unsigned int> vertices;
    for(unsigned int i=0;i<5;i++)vertices.push_back(i);
    graph::container::compressed_graph<int, unsigned int> astGraph{{0,1,1}, {0,3,1}, {1,2,1}, {3,4,1}};//, {0,5,1}, {5,6,1}, {0,7,1}, {7,8,1}, {0,9,1}, {9,10,1}, {0,11,1}, {11,12,1}, {0,13,1}, {13,14,1}, {14,15,1}, {15,16,1}, {14,17,1}, {17,18,1}, {14,19,1}, {0,20,1}, {21,20,1}, {21,22,1}, {22,23,1}, {21,24,1}, {24,25,1}, {21,26,1}, {26,27,1}, {21,28,1}, {0,29,1}, {29,30,1}, {30,31,1}, {31,32,1}, {30,33,1}, {33,34,1}, {30,35,1}, {35,36,1}, {30,37,1}, {37,38,1}, {30,39,1}, {39,40,1}, {30,41,1}, {0,42,1}, {42,43,1}, {0,44,1}, {44,45,1}, {0,46,1}};
    //astGraph.reserve(3);
    astGraph.load_vertices(vertices, [&vertices](unsigned int& nm) {
        auto uid = static_cast<graph::vertex_id_t<graph::container::compressed_graph<int, unsigned int>>>(&nm - vertices.data());
        //std::cout<<"uid "<<uid<<std::endl;
        return graph::copyable_vertex_t< graph::vertex_id_t<graph::container::compressed_graph<int, unsigned int>>, unsigned int>{uid, nm};
    });
    //std::cout<<"size: "<<graph::num_vertices(astGraph)<<" "<<graph::vertices(astGraph).size()<<std::endl;
    CHECK_EQ(graph::num_vertices(astGraph), 5);
    CHECK_EQ(graph::vertices(astGraph).size(), 5);
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
    jsonBinder.display();
    sylvanmats::io::json::Path jpName;
    jpName["elements"]["*"]["*"]["symbol"]=="H";
    size_t val=0;
    jsonBinder(jpName, [&](std::string_view& key, std::any& v){
        std::cout<<"key "<<key<<std::endl;
        if(key.compare("number")==0){
                   val=std::any_cast<long>(v);
        }
    });
    CHECK_EQ(val, 1);
    sylanmats::io::tikz::GraphPublisher graphPublisher;
    std::string&& tikzDrawing=graphPublisher(jsonBinder);
    std::filesystem::path filePath="../documents/json_graph.tex";
    std::ofstream ofs(filePath);
    ofs<<tikzDrawing<<std::endl;
    ofs.close();
    
}

TEST_CASE("test create json" * doctest::skip()) {
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

TEST_CASE("test create series json" * doctest::skip()) {
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
std::cout<<"bind package "<<std::endl;
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
        CHECK_EQ(graph::num_vertices(jsonBinder.dagGraph), 47);
        CHECK_EQ(graph::num_edges(jsonBinder.dagGraph), 46);
                    /*std::cout<<"display rep graph "<<std::endl;
   directedness dir=directedness::directed;
   std::string_view arrows, rev_arrows = "dir=back,arrowhead=vee,";
  for (auto&& [uid, u] : graph::views::vertexlist(jsonBinder.dagGraph)) {
    std::cout << "  " << uid << " [label=\"" << jsonBinder.jsonContent.substr(graph::vertex_value(jsonBinder.dagGraph, u).start, graph::vertex_value(jsonBinder.dagGraph, u).end-graph::vertex_value(jsonBinder.dagGraph, u).start) << " [" << uid << "]\"]\n";
    for (auto&& [vid, uv] : graph::views::incidence(jsonBinder.dagGraph, uid)) {
      auto&&           v   = graph::target(jsonBinder.dagGraph, uv);
      std::string_view arw = (dir == directedness::directed && vid < uid) ? rev_arrows : "";
      std::cout << "   " << uid << " -> " << vid << " [" << arw << "xlabel=\"" << graph::edge_value(jsonBinder.dagGraph, uv)<<" "<<jsonBinder.jsonContent.substr(graph::vertex_value(jsonBinder.dagGraph, v).start, graph::vertex_value(jsonBinder.dagGraph, v).end-graph::vertex_value(jsonBinder.dagGraph, v).start) << " \"]\n";
    }
    std::cout << std::endl;
  }*/
        
        
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
