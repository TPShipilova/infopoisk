#ifndef BINARY_SEARCH_TREE_HPP
#define BINARY_SEARCH_TREE_HPP

#include <functional>
#include <vector>
#include <string>
#include <iostream>

template<typename K, typename V>
class BinarySearchTree {
private:
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        int height;

        Node(const K& k, const V& v)
            : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
    };

    Node* root;
    size_t count;

    // AVL функции балансировки
    int height(Node* node) const {
        return node ? node->height : 0;
    }

    int balance_factor(Node* node) const {
        return node ? height(node->left) - height(node->right) : 0;
    }

    void update_height(Node* node) {
        if (node) {
            node->height = 1 + std::max(height(node->left), height(node->right));
        }
    }

    Node* rotate_right(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;

        x->right = y;
        y->left = T2;

        update_height(y);
        update_height(x);

        return x;
    }

    Node* rotate_left(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;

        y->left = x;
        x->right = T2;

        update_height(x);
        update_height(y);

        return y;
    }

    Node* balance(Node* node) {
        if (!node) return nullptr;

        update_height(node);
        int bf = balance_factor(node);

        // Left Left
        if (bf > 1 && balance_factor(node->left) >= 0) {
            return rotate_right(node);
        }

        // Right Right
        if (bf < -1 && balance_factor(node->right) <= 0) {
            return rotate_left(node);
        }

        // Left Right
        if (bf > 1 && balance_factor(node->left) < 0) {
            node->left = rotate_left(node->left);
            return rotate_right(node);
        }

        // Right Left
        if (bf < -1 && balance_factor(node->right) > 0) {
            node->right = rotate_right(node->right);
            return rotate_left(node);
        }

        return node;
    }

    Node* insert(Node* node, const K& key, const V& value) {
        if (!node) {
            count++;
            return new Node(key, value);
        }

        if (key < node->key) {
            node->left = insert(node->left, key, value);
        } else if (key > node->key) {
            node->right = insert(node->right, key, value);
        } else {
            node->value = value;
            return node;
        }

        return balance(node);
    }

    Node* find_min(Node* node) const {
        while (node && node->left) {
            node = node->left;
        }
        return node;
    }

    Node* remove(Node* node, const K& key) {
        if (!node) return nullptr;

        if (key < node->key) {
            node->left = remove(node->left, key);
        } else if (key > node->key) {
            node->right = remove(node->right, key);
        } else {
            if (!node->left || !node->right) {
                Node* temp = node->left ? node->left : node->right;

                if (!temp) {
                    temp = node;
                    node = nullptr;
                } else {
                    *node = *temp;
                }

                delete temp;
                count--;
            } else {
                Node* temp = find_min(node->right);
                node->key = temp->key;
                node->value = temp->value;
                node->right = remove(node->right, temp->key);
            }
        }

        if (!node) return nullptr;

        return balance(node);
    }

    Node* search(Node* node, const K& key) const {
        if (!node || node->key == key) {
            return node;
        }

        if (key < node->key) {
            return search(node->left, key);
        }

        return search(node->right, key);
    }

    void in_order_traversal(Node* node, std::vector<std::pair<K, V>>& result) const {
        if (!node) return;

        in_order_traversal(node->left, result);
        result.emplace_back(node->key, node->value);
        in_order_traversal(node->right, result);
    }

    void destroy(Node* node) {
        if (!node) return;

        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    BinarySearchTree() : root(nullptr), count(0) {}
    ~BinarySearchTree() { destroy(root); }

    void insert(const K& key, const V& value) {
        root = insert(root, key, value);
    }

    bool remove(const K& key) {
        size_t old_count = count;
        root = remove(root, key);
        return count < old_count;
    }

    bool contains(const K& key) const {
        return search(root, key) != nullptr;
    }

    bool get(const K& key, V& value) const {
        Node* node = search(root, key);
        if (node) {
            value = node->value;
            return true;
        }
        return false;
    }

    std::vector<std::pair<K, V>> get_all_sorted() const {
        std::vector<std::pair<K, V>> result;
        result.reserve(count);
        in_order_traversal(root, result);
        return result;
    }

    size_t size() const { return count; }
    bool empty() const { return count == 0; }

    void clear() {
        destroy(root);
        root = nullptr;
        count = 0;
    }
};

#endif