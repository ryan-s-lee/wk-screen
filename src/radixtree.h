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
    struct node
    {
        std::unordered_map<std::string, std::shared_ptr<node<T>>> children;
        std::unique_ptr<T> value;
        node(T *_value) : children() {
            value = std::unique_ptr<T>(_value);
        }

        // default destructor
        // ~node() {}
    };

    template <typename T>
    class radix_tree
    {
    private:
        /**
         * The root node of the trie.
        */
        std::shared_ptr<node<T>> root; // 
        std::tuple<std::shared_ptr<node<T>>, std::string_view, std::string> get_node(std::string_view query);
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
        root = std::make_shared<node<T>>(nullptr);
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
    std::tuple<std::shared_ptr<node<T>>, std::string_view, std::string> radix_tree<T>::get_node(std::string_view query) 
    {
        // Find the node with the longest matching prefix (the LMP node)
        std::shared_ptr<node<T>> selected = std::shared_ptr(this->root);
        auto child_itr = selected->children.begin(); 

        while(child_itr != selected->children.end())
        {
            auto itrs = std::mismatch(
                child_itr->first.begin(), child_itr->first.end(), query.begin(), query.end()
            );
            // If a prefix was completely matched, 
            // enter the prefix-matched node and repeat the lcp search.
            if (itrs.first == child_itr->first.end()) {
            #ifdef DEBUG
                std::cout << child_itr->first << "---";
            #endif
                selected = child_itr->second;
                query.remove_prefix(child_itr->first.size());
                // TODO: clear itrs, so that if we enter a leaf node we still fail.
                child_itr = selected->children.begin();
            }             // If a partial match was found, we cannot progress further
            else if (itrs.first != child_itr->first.begin()) {
            #ifdef DEBUG
                std::cout << child_itr->first << '\n';
            #endif
                return {selected, std::string_view(child_itr->first), std::string(query)}; // NOTE: query should have move semantics
            } else {
                ++child_itr;
            } 

            // If there was no partial match, check another edge.
        }
    #ifdef DEBUG
        std::cout << '\n';
    #endif
        return {selected, std::string_view(), std::string(query)};
    }

    template <typename T>
    bool radix_tree<T>::put(const std::string_view &query, T *value)
    {
        // obtain the node to add to (in result.first) and its edge label
        // (in result.second) for checking partial matches/splitting the edge
        std::shared_ptr<node<T>> node_to_add_to;
        std::string_view edge_lbl;
        std::string remaining_query;
        std::tie(node_to_add_to, edge_lbl, remaining_query) = this->get_node(query);
    #ifdef DEBUG
        std::cout << "edge_lbl=" << edge_lbl << " | remaining_query=" << remaining_query << '\n';
    #endif
        auto prefix_itrs = std::mismatch(remaining_query.begin(), remaining_query.end(), edge_lbl.begin(), edge_lbl.end());

        // If there was a partial match among the children of the LMP node, 
        // make a new node for that prefix. 
        if (edge_lbl.length() > 0){
            std::shared_ptr<node<T>> bridge_node = std::make_shared<node<T>>(nullptr);

            // Assign the partially matched node as a child of the new bridge node,
            // with its edge/key labelled as the remaining suffix after removing
            // the partial match
            std::string oldedge_suf(prefix_itrs.second, edge_lbl.end());
#ifdef DEBUG
            std::cout << "old edge suffix: " << oldedge_suf << '\n';
#endif
            bridge_node->children.insert({
                {
                    oldedge_suf, 
                    node_to_add_to->children[std::string(edge_lbl)] // the original child node
                } 
            });

            // Remove the old edge on the parent with the partially matched key.
            node_to_add_to->children.erase(std::string(edge_lbl));

            // Assign the bridge node to the original parent node
            std::string oldedge_pref(edge_lbl.begin(), prefix_itrs.second);
#ifdef DEBUG
            std::cout << "old edge prefix: " << oldedge_pref << '\n';
#endif
            node_to_add_to->children.insert({
                {oldedge_pref, bridge_node}
            });

            // make the bridge node the new parent, to which we add the other element
            node_to_add_to = bridge_node;
        }
        std::string newedge(prefix_itrs.first, remaining_query.end());
    #ifdef DEBUG
        std::cout << "adding edge: " << newedge << '\n';
    #endif
        node_to_add_to->children.insert({{newedge, std::make_shared<node<T>>(value)}});
        return 0;
    }

    template <typename T>
    bool radix_tree<T>::evict_lru(){
        return true;
    }

    template <typename T>
    T radix_tree<T>::get(const std::string_view &query)
    {
        // Find the child with longest common prefix
        std::shared_ptr<node<T>> obtained_node;
        std::tie(obtained_node, std::ignore, std::ignore) = get_node(query);
        if (obtained_node->value == nullptr) {
            return T();
        }

        return *(obtained_node->value);
    }

    template <typename T>
    std::pair<T, std::string> radix_tree<T>::get_best_match(std::string_view query) {
        // Find the node with the longest matching prefix (the LMP node)
        std::shared_ptr<node<T>> selected(this->root);
        auto child_itr = selected->children.begin(); 

        // During traversal, keep track of the best match so far.
        std::string prefix_so_far("");
        unsigned long best_len = 0;
        std::shared_ptr<node<T>> best_node(selected);

        while(child_itr != selected->children.end())
        {
            auto itrs = std::mismatch(
                child_itr->first.begin(), child_itr->first.end(), query.begin(), query.end()
            );

            // If a prefix was completely matched, 
            // enter the prefix-matched node and repeat the LMP search.
            // Also, update the best so far
            if (itrs.first == child_itr->first.end()) {
                prefix_so_far += std::string(child_itr->first.begin(), itrs.first);
                if (selected->value != nullptr) {
                    best_len = prefix_so_far.length();
                    best_node = selected;
                }
                selected = child_itr->second;
                query.remove_prefix(child_itr->first.size());
                // TODO: clear itrs, so that if we enter a leaf node we still fail.
                child_itr = selected->children.begin();
            }
            else if (itrs.first != child_itr->first.begin()) {
                break;
            } else {
                ++child_itr;
            } 

            // If there was no partial match, check another edge.
        }
        
        auto final_val = best_node->value == nullptr ? T() : *(best_node->value);
        return std::pair(final_val, std::string(prefix_so_far, best_len));
    }
    
} // namespace llm_sys
