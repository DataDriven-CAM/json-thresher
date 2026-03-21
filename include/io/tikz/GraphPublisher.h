#pragma once

#include <string>
#include <iostream>
#include <filesystem>
#include <typeinfo>

#include "io/json/Binder.h"
#include "graph/views/dfs.hpp"
#include "graph/views/vertexlist.hpp"

#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "fmt/ranges.h"

namespace sylanmats::io::tikz{

    class GraphPublisher{
    protected:
        std::string graphTemplate{};
    public:
        GraphPublisher(){
            std::string templateLocation=(getenv("JSONTHRESHER_DB_LOCATION")!=nullptr) ? std::string(getenv("JSONTHRESHER_DB_LOCATION"))+"/templates/tikz": "./templates/tikz";
            std::filesystem::path path=templateLocation+"/tikz.txt";
            std::ifstream file(path);
            graphTemplate=std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        };
        GraphPublisher(const GraphPublisher& orig) = delete;
        virtual ~GraphPublisher() = default;
    
        std::string operator ()(sylvanmats::io::json::Binder& jsonBinder){
            std::string tree;
            std::vector<int> visited(graph::num_edges(jsonBinder.dagGraph), 0);
            auto vlist = graph::views::vertexlist(jsonBinder.dagGraph);
            for (auto&& [uid, u] : graph::views::vertexlist(jsonBinder.dagGraph)) {
//                auto uid=graph::vertex_id(jsonBinder.dagGraph, u);
                if (visited[uid]>0 || size(graph::views::incidence(jsonBinder.dagGraph, uid))==0) {
                  continue;
                }
                visited[uid]++;
                auto uValue=graph::vertex_value(jsonBinder.dagGraph, *graph::find_vertex(jsonBinder.dagGraph, uid));
                tree.append(std::to_string(uValue.id));
                if(size(graph::views::incidence(jsonBinder.dagGraph, uid))>1)tree.append("{");
                auto dfs=graph::views::edges_dfs(jsonBinder.dagGraph, uid);
                size_t depth=dfs.depth();
                size_t notchDepth=depth;
                for (auto&& [e] : dfs) {
                    auto vid = static_cast<size_t>(graph::source_id(jsonBinder.dagGraph, e));
                    auto wid = static_cast<size_t>(graph::target_id(jsonBinder.dagGraph, e));
                  //auto ev=edge_value(jsonBinder.dagGraph, wv);
                  //std::cout<<typeid(wv).name()<<" "<<depth<<" "<<dfs.depth()<<" "<<size(graph::edges(jsonBinder.dagGraph, vid))<<" "<<visited[vid]<<" "<<size(graph::edges(jsonBinder.dagGraph, wid))<<" "<<visited[wid]<<std::endl;
                  //if (!visited[ev]) {
                  if(depth>dfs.depth()){
                    if(!tree.empty() && size(graph::views::incidence(jsonBinder.dagGraph, vid))>1 && size(graph::views::incidence(jsonBinder.dagGraph, vid))==visited[vid])tree.append("}");
                    if(!tree.empty())tree.append(";\n");
                    auto vValue=graph::vertex_value(jsonBinder.dagGraph, *graph::find_vertex(jsonBinder.dagGraph, vid));
                    tree.append(std::to_string(vValue.id));
                  }
                    auto& wValue=graph::vertex_value(jsonBinder.dagGraph, *graph::find_vertex(jsonBinder.dagGraph, wid));
                    tree.append(" -> ");
                    if(depth<dfs.depth() && size(graph::views::incidence(jsonBinder.dagGraph, vid))>1 && visited[vid]==0)tree.append("{");
                    tree.append("\"");
                    if(wValue.obj_type==sylvanmats::io::json::PAIR_KEY){
                        std::string pairKeyName(substr_view(jsonBinder.jsonContent, wValue.start, wValue.end));
                        size_t pos=std::string::npos;
                        while ((pos = pairKeyName.find("_")) != std::string::npos) {
                            pairKeyName.replace(pos, 1, R"(-)");
                        }
                        tree.append(pairKeyName);
                    }
                    tree.append(std::to_string(wValue.id));
                    tree.append("\"");
                    if(size(graph::views::incidence(jsonBinder.dagGraph, wid))==0)tree.append(" [mark]");
                    visited[vid]++;
                  //}
                  if(depth>dfs.depth())notchDepth=dfs.depth();
                  depth=dfs.depth();
                }
                    tree.append("};\n");
            }
            auto cArg=fmt::arg("tree", tree);
            //auto dArg=fmt::arg("leafs", indices);
            std::string ret=fmt::vformat(graphTemplate, fmt::make_format_args(cArg));
            return ret;
        };
    };
}
