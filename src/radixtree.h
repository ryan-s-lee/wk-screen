#include <string_view>
#ifdef DEBUG
#include <iostream>
#endif
#include <string>
#include <tuple>
#include <unordered_map>
#include <memory>
#include <utility>



namespace llm_sys
{


template <typename T>
class radix_tree
{
private:
    struct node
    {

        // edges are represented by a pair containing a prefix and a node, 
        // but these edges are indexed by the first character of the prefix.
        std::unordered_map<char, std::pair<std::string, std::shared_ptr<node>>> children;
        std::unique_ptr<T> value;
        node(T *_value) : children() {
            value = std::unique_ptr<T>(_value);
        }

        // default destructor
        // ~node() {}
    };
    /**
     * The root node of the trie.
    */
    std::shared_ptr<node> root; // 
    std::tuple<std::shared_ptr<node>, std::string, std::string> get_node(std::string_view query);
public:
    radix_tree();
    ~radix_tree();
    bool put(const std::string_view &prefix, T *value);
    T get(const std::string_view &query);
    bool evict_lru();
    std::pair<T, std::string> get_best_match(std::string_view query);
};

template <typename T>
radix_tree<T>::radix_tree(/* args */) 
{
    root = std::make_shared<node>(nullptr);
}

template <typename T>
radix_tree<T>::~radix_tree()
{
    // recursively free nodes from the leaves upward
}

/**
* Returns the node that best matches the query, the best-matching edge at the 
* best node (or an empty string view if there), and the remaining prefix not consumed
* during the search.
*/
template <typename T>
std::tuple<std::shared_ptr<typename radix_tree<T>::node>, std::string, std::string> radix_tree<T>::get_node(std::string_view query) 
{
    // Find the node with the longest matching prefix (the LMP node)
    std::shared_ptr<node> selected = std::shared_ptr(this->root);

    // find the index with the matching prefix based on the first character.
    while(true) {

        // get the next edge
        auto result = selected->children.find(query[0]);

        // if the next edge does not exist, we've found the best node we could
        if (result == selected->children.end()) {
            #ifdef DEBUG
            std::cout << '\n';
            #endif
            return {selected, std::string(), std::string(query)};
        }

        // extract information from the selected iterator
        std::string edge_lbl;
        std::shared_ptr<node> next;
        std::tie(edge_lbl, next) = result->second;

        // get the matching prefix/different suffixes of the edge label and
        // the remaining suffix of the query
        std::string::iterator edge_diff_itr;
        std::string_view::iterator query_diff_itr;
        std::tie(edge_diff_itr, query_diff_itr) = std::mismatch(
            edge_lbl.begin(), edge_lbl.end(), query.begin(), query.end()
        );

        // If the prefix was completely matched, 
        // enter the prefix-matched node and repeat the lcp search.
        if (edge_diff_itr == edge_lbl.end()) {
            #ifdef DEBUG
            std::cout << edge_lbl << "---";
            #endif
            selected = next;
            query.remove_prefix(edge_lbl.length());
        }             
        else {
            // If only a partial match was found, we cannot progress further
            #ifdef DEBUG
            std::cout << edge_lbl << '\n';
            #endif
            return {selected, edge_lbl, std::string(query)}; // NOTE: query should have move semantics
        }
    }
#ifdef DEBUG
    std::cout << '\n';
#endif
    return {selected, std::string(), std::string(query)};
}



template <typename T>
bool radix_tree<T>::put(const std::string_view &query, T *value)
{
    if (query.length() <= 0) {
        return false;
    }
    // obtain the node to add to (in result.first) and its edge label
    // (in result.second) for checking partial matches/splitting the edge
    std::shared_ptr<node> node_to_add_to;
    std::string edge_lbl; // this is only a view of the string contained with the node
    std::string remaining_query;
    std::tie(node_to_add_to, edge_lbl, remaining_query) = this->get_node(query);
#ifdef DEBUG
    std::cout << "edge_lbl=" << edge_lbl << " | remaining_query=" << remaining_query << '\n';
#endif
    std::string::iterator query_diff_itr;
    std::string::iterator edge_diff_itr;
    std::tie(query_diff_itr, edge_diff_itr) 
        = std::mismatch(remaining_query.begin(), remaining_query.end(), edge_lbl.begin(), edge_lbl.end());


    // If there was a partial match among the children of the LMP node, 
    // make a new node for that prefix. 
    if (edge_lbl.length() > 0){
        std::shared_ptr<node> bridge_node = std::make_shared<node>(nullptr);


        // get shared prefixes
        std::string oldedge_suffix(edge_diff_itr, edge_lbl.end());
#ifdef DEBUG
        std::cout << "old edge suffix: " << oldedge_suffix << '\n';
#endif
        std::string shared_prefix(edge_lbl.begin(), edge_diff_itr);
#ifdef DEBUG
        std::cout << "shared prefix: " << shared_prefix << '\n';
#endif

        // Assign the partially matched node as a child of the new bridge node,
        // with its edge/key labelled as the remaining suffix after removing
        // the partial match
        bridge_node->children.insert(
            std::make_pair(
                oldedge_suffix[0], 
                std::make_pair(oldedge_suffix, node_to_add_to->children[edge_lbl[0]].second) // the original child node
            )
        );

        // Remove the old edge on the parent with the partially matched key.
        node_to_add_to->children.erase(edge_lbl[0]);

        // Assign the bridge node to the original parent node
        node_to_add_to->children.insert(std::make_pair(
            shared_prefix[0], std::make_pair(shared_prefix, bridge_node)
        ));

        // make the bridge node the new parent, to which we add the other element
        node_to_add_to = bridge_node;
    }
    std::string newedge(query_diff_itr, remaining_query.end());
#ifdef DEBUG
    std::cout << "adding edge: " << newedge << '\n';
#endif
    node_to_add_to->children.insert({
        {newedge[0], std::make_pair(newedge, std::make_shared<node>(value))}
    });
    return true;
}



template <typename T>
bool radix_tree<T>::evict_lru(){
    return true;
}



template <typename T>
T radix_tree<T>::get(const std::string_view &query)
{
    // Find the child with longest common prefix
    std::shared_ptr<node> obtained_node;
    std::tie(obtained_node, std::ignore, std::ignore) = get_node(query);
    if (obtained_node->value == nullptr) {
        return T();
    }

    return *(obtained_node->value);
}



template <typename T>
std::pair<T, std::string> radix_tree<T>::get_best_match(std::string_view query) {
    // Find the node with the longest matching prefix (the LMP node)
    std::shared_ptr<node> selected = std::shared_ptr(this->root);
    std::shared_ptr<node> best_so_far = selected;

    // find the index with the matching prefix based on the first character.
    while(true) {
        if (selected->value) {
            best_so_far = selected;
        }

        // get the next edge
        auto result = selected->children.find(query[0]);

        // if the next edge does not exist, we've found the best node we could
        if (result == selected->children.end()) {
            #ifdef DEBUG
            std::cout << '\n';
            #endif
            break;
        }

        // extract information from the selected iterator
        std::string edge_lbl;
        std::shared_ptr<node> next;
        std::tie(edge_lbl, next) = result->second;

        // get the matching prefix/different suffixes of the edge label and
        // the remaining suffix of the query
        std::string::iterator edge_diff_itr;
        std::string_view::iterator query_diff_itr;
        std::tie(edge_diff_itr, query_diff_itr) = std::mismatch(
            edge_lbl.begin(), edge_lbl.end(), query.begin(), query.end()
        );

        // If the prefix was completely matched, 
        // enter the prefix-matched node and repeat the lcp search.
        if (edge_diff_itr == edge_lbl.end()) {
            #ifdef DEBUG
            std::cout << edge_lbl << "---";
            #endif
            selected = next;
            query.remove_prefix(edge_lbl.length());
        }             
        else {
            // If only a partial match was found, we cannot progress further
            #ifdef DEBUG
            std::cout << edge_lbl << '\n';
            #endif
            break;
        }
    }
#ifdef DEBUG
    std::cout << '\n';
#endif
    T final_val = (best_so_far->value ? *best_so_far->value : T());
    return std::make_pair(final_val, std::string());
}



} // namespace llm_sys

extern "C"{
extern void test(void);
}
