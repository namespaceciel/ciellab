#ifndef CIELLAB_INCLUDE_CIEL_CORE_RB_TREE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_RB_TREE_HPP_

#include <ciel/core/compressed_pair.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>
#include <ciel/core/packed_ptr.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

NAMESPACE_CIEL_BEGIN

// Inspired by LLVM libc++'s implementation.

template<class Derived>
struct rb_end_node {
    using pointer = Derived*;

    pointer left;

}; // struct rb_end_node

template<class Derived>
struct rb_node_base : rb_end_node<Derived> {
    using pointer      = Derived*;
    using base_type    = rb_end_node<Derived>;
    using base_pointer = base_type*;

    pointer right;
    uintptr_t parent_ : 63;
    bool is_black     : 1;

    CIEL_NODISCARD base_pointer parent() const noexcept {
        return reinterpret_cast<base_pointer>(parent_);
    }

    CIEL_NODISCARD pointer parent_downcast() const noexcept {
        return static_cast<pointer>(parent());
    }

    void set_parent(base_pointer p) noexcept {
        parent_ = reinterpret_cast<uintptr_t>(p);
    }

}; // struct rb_node

template<class T>
struct rb_node : rb_node_base<rb_node<T>> {
    using value_type = T;

    using base_type = rb_node_base<rb_node<T>>;
    using base_type::is_black;
    using base_type::left;
    using base_type::parent;
    using base_type::parent_downcast;
    using base_type::right;
    using base_type::set_parent;

private:
    value_type value_;

public:
    template<class... Args>
    rb_node(Args&&... args) noexcept(std::is_nothrow_constructible<value_type, Args&&...>::value)
        : value_(std::forward<Args>(args)...) {}

    CIEL_NODISCARD value_type& value() noexcept {
        return value_;
    }

    CIEL_NODISCARD const value_type& value() const noexcept {
        return value_;
    }

}; // struct rb_node

template<>
struct rb_node<void> : rb_node_base<rb_node<void>> {
    using value_type = uintptr_t;

    CIEL_NODISCARD value_type value() const noexcept {
        return reinterpret_cast<value_type>(this);
    }

}; // struct rb_node<void>

template<class TreeNodePtr>
CIEL_NODISCARD bool is_left_child(TreeNodePtr p) noexcept {
    CIEL_ASSERT(p != nullptr);

    return p == p->parent()->left;
}

template<class TreeNodePtr>
CIEL_NODISCARD TreeNodePtr max(TreeNodePtr p) noexcept {
    CIEL_ASSERT(p != nullptr);

    while (p->right != nullptr) {
        p = p->right;
    }

    return p;
}

template<class TreeEndNodePtr>
CIEL_NODISCARD TreeEndNodePtr min(TreeEndNodePtr p) noexcept {
    CIEL_ASSERT(p != nullptr);

    while (p->left != nullptr) {
        p = p->left;
    }

    return p;
}

template<class TreeNodePtr, class TreeEndNodePtr = decltype(std::declval<TreeNodePtr>()->parent())>
CIEL_NODISCARD TreeEndNodePtr next(TreeNodePtr p) noexcept {
    CIEL_ASSERT(p != nullptr);

    if (p->right != nullptr) {
        return ciel::min(p->right);
    }

    while (!ciel::is_left_child(p)) {
        // If p is right child, then p->parent() is not end_node.
        p = p->parent_downcast();
    }

    return p->parent();
}

template<class TreeEndNodePtr, class TreeNodePtr = decltype(std::declval<TreeEndNodePtr>()->left)>
CIEL_NODISCARD TreeNodePtr prev(TreeEndNodePtr p) noexcept {
    CIEL_ASSERT(p != nullptr);

    if (p->left != nullptr) {
        return ciel::max(p->left);
    }

    // If p doesn't have left child and still has prev, then p is not end_node.
    TreeNodePtr ptr = static_cast<TreeNodePtr>(p);

    // ptr will never be end_node, so it's safe to call parent_downcast.
    while (ciel::is_left_child(ptr)) {
        ptr = ptr->parent_downcast();
    }

    return ptr->parent_downcast();
}

template<class TreeNodePtr>
void left_rotate(TreeNodePtr p) noexcept {
    CIEL_ASSERT(p != nullptr);
    CIEL_ASSERT(p->right != nullptr);

    TreeNodePtr r = p->right;
    p->right      = r->left;

    if (p->right != nullptr) {
        p->right->set_parent(p);
    }

    r->set_parent(p->parent());

    if (ciel::is_left_child(p)) {
        p->parent()->left = r;

    } else {
        p->parent_downcast()->right = r;
    }

    r->left = p;
    p->set_parent(r);
}

template<class TreeNodePtr>
void right_rotate(TreeNodePtr p) noexcept {
    CIEL_ASSERT(p != nullptr);
    CIEL_ASSERT(p->left != nullptr);

    TreeNodePtr l = p->left;
    p->left       = l->right;

    if (p->left != nullptr) {
        p->left->set_parent(p);
    }

    l->set_parent(p->parent());

    if (ciel::is_left_child(p)) {
        p->parent()->left = l;

    } else {
        p->parent_downcast()->right = l;
    }

    l->right = p;
    p->set_parent(l);
}

template<class TreeNodePtr>
void balance_after_insert(TreeNodePtr root, TreeNodePtr p) noexcept {
    CIEL_ASSERT(root != nullptr);
    CIEL_ASSERT(p != nullptr);

    p->is_black = (p == root);

    while (p != root && !p->parent_downcast()->is_black) {
        if (ciel::is_left_child(p->parent_downcast())) {
            TreeNodePtr y = p->parent_downcast()->parent_downcast()->right;

            if (y != nullptr && !y->is_black) {
                p           = p->parent_downcast();
                p->is_black = true;
                p           = p->parent_downcast();
                p->is_black = (p == root);
                y->is_black = true;

            } else {
                if (!ciel::is_left_child(p)) {
                    p = p->parent_downcast();
                    ciel::left_rotate(p);
                }

                p           = p->parent_downcast();
                p->is_black = true;
                p           = p->parent_downcast();
                p->is_black = false;
                ciel::right_rotate(p);
                break;
            }

        } else {
            TreeNodePtr y = p->parent_downcast()->parent()->left;

            if (y != nullptr && !y->is_black) {
                p           = p->parent_downcast();
                p->is_black = true;
                p           = p->parent_downcast();
                p->is_black = (p == root);
                y->is_black = true;

            } else {
                if (ciel::is_left_child(p)) {
                    p = p->parent_downcast();
                    ciel::right_rotate(p);
                }

                p           = p->parent_downcast();
                p->is_black = true;
                p           = p->parent_downcast();
                p->is_black = false;
                ciel::left_rotate(p);
                break;
            }
        }
    }
}

template<class TreeNodePtr>
void remove(TreeNodePtr root, TreeNodePtr z) noexcept {
    CIEL_ASSERT(root != nullptr);
    CIEL_ASSERT(z != nullptr);

    TreeNodePtr y = (z->left == nullptr || z->right == nullptr) ? z : static_cast<TreeNodePtr>(ciel::next(z));
    TreeNodePtr p = (y->left != nullptr) ? y->left : y->right;
    TreeNodePtr w = nullptr;

    if (p != nullptr) {
        p->set_parent(y->parent());
    }
    if (ciel::is_left_child(y)) {
        y->parent()->left = p;
        if (y != root) {
            w = y->parent_downcast()->right;

        } else {
            root = p;
        }

    } else {
        y->parent_downcast()->right = p;
        w                           = y->parent()->left;
    }

    const bool removed_black = y->is_black;

    if (y != z) {
        y->set_parent(z->parent());

        if (ciel::is_left_child(z)) {
            y->parent()->left = y;

        } else {
            y->parent_downcast()->right = y;
        }

        y->left = z->left;
        y->left->set_parent(y);
        y->right = z->right;

        if (y->right != nullptr) {
            y->right->set_parent(y);
        }

        y->is_black = z->is_black;

        if (root == z) {
            root = y;
        }
    }

    if (!removed_black || root == nullptr) {
        return;
    }

    if (p != nullptr) {
        p->is_black = true;
        return;
    }

    while (true) {
        if (!ciel::is_left_child(w)) {
            if (!w->is_black) {
                w->is_black                    = true;
                w->parent_downcast()->is_black = false;
                ciel::left_rotate(w->parent_downcast());

                if (root == w->left) {
                    root = w;
                }

                w = w->left->right;
            }

            if ((w->left == nullptr || w->left->is_black) && (w->right == nullptr || w->right->is_black)) {
                w->is_black = false;
                p           = w->parent_downcast();

                if (p == root || !p->is_black) {
                    p->is_black = true;
                    break;
                }

                w = ciel::is_left_child(p) ? p->parent_downcast()->right : p->parent()->left;

            } else {
                if (w->right == nullptr || w->right->is_black) {
                    w->left->is_black = true;
                    w->is_black       = false;
                    ciel::right_rotate(w);
                    w = w->parent_downcast();
                }

                w->is_black                    = w->parent_downcast()->is_black;
                w->parent_downcast()->is_black = true;
                w->right->is_black             = true;
                ciel::left_rotate(w->parent_downcast());
                break;
            }

        } else {
            if (!w->is_black) {
                w->is_black                    = true;
                w->parent_downcast()->is_black = false;
                ciel::right_rotate(w->parent_downcast());

                if (root == w->right) {
                    root = w;
                }

                w = w->right->left;
            }

            if ((w->left == nullptr || w->left->is_black) && (w->right == nullptr || w->right->is_black)) {
                w->is_black = false;
                p           = w->parent_downcast();

                if (!p->is_black || p == root) {
                    p->is_black = true;
                    break;
                }

                w = ciel::is_left_child(p) ? p->parent_downcast()->right : p->parent()->left;

            } else {
                if (w->left == nullptr || w->left->is_black) {
                    w->right->is_black = true;
                    w->is_black        = false;
                    ciel::left_rotate(w);
                    w = w->parent_downcast();
                }

                w->is_black                    = w->parent_downcast()->is_black;
                w->parent_downcast()->is_black = true;
                w->left->is_black              = true;
                ciel::right_rotate(w->parent_downcast());
                break;
            }
        }
    }
}

template<class T>
class rb_iterator {
private:
    using TreeNode       = rb_node<T>;
    using TreeNodePtr    = TreeNode*;
    using TreeEndNode    = rb_end_node<TreeNode>;
    using TreeEndNodePtr = TreeEndNode*;

    using value_type = typename TreeNode::value_type;

    TreeEndNodePtr ptr_;

public:
    rb_iterator(TreeEndNodePtr p) noexcept
        : ptr_(p) {}

    rb_iterator& operator++() noexcept {
        ptr_ = ciel::next(static_cast<TreeNodePtr>(ptr_));
        return *this;
    }

    CIEL_NODISCARD rb_iterator operator++(int) noexcept {
        auto res = *this;
        ++(*this);
        return res;
    }

    rb_iterator& operator--() noexcept {
        ptr_ = ciel::prev(ptr_);
        return *this;
    }

    CIEL_NODISCARD rb_iterator operator--(int) noexcept {
        auto res = *this;
        --(*this);
        return res;
    }

    CIEL_NODISCARD auto operator*() const noexcept -> decltype(std::declval<TreeNode>().value()) {
        return static_cast<TreeNodePtr>(ptr_)->value();
    }

    CIEL_NODISCARD friend bool operator==(const rb_iterator lhs, const rb_iterator rhs) noexcept {
        return lhs.ptr_ == rhs.ptr_;
    }

    CIEL_NODISCARD friend bool operator!=(const rb_iterator lhs, const rb_iterator rhs) noexcept {
        return !(lhs == rhs);
    }

}; // class rb_iterator

template<class T, class Compare>
class rb_tree {
private:
    using TreeNode       = rb_node<T>;
    using TreeNodePtr    = TreeNode*;
    using TreeEndNode    = rb_end_node<TreeNode>;
    using TreeEndNodePtr = TreeEndNode*;

    using value_type    = typename TreeNode::value_type;
    using size_type     = size_t;
    using value_compare = Compare;
    using iterator      = rb_iterator<T>;

    TreeEndNodePtr begin_{&end_node_};
    TreeEndNode end_node_{nullptr};
    compressed_pair<size_type, value_compare> size_comp_{0, value_init};

    CIEL_NODISCARD size_type& size_() noexcept {
        return size_comp_.first();
    }

    CIEL_NODISCARD const size_type& size_() const noexcept {
        return size_comp_.first();
    }

    CIEL_NODISCARD value_compare& comp_() noexcept {
        return size_comp_.second();
    }

    CIEL_NODISCARD const value_compare& comp_() const noexcept {
        return size_comp_.second();
    }

    TreeNodePtr root() noexcept {
        return end_node_.left;
    }

    TreeNodePtr* root_ptr() noexcept {
        return &(end_node_.left);
    }

    // Return reference to the insert place and it's parent: <Parent, Child>
    std::pair<TreeEndNodePtr, TreeNodePtr&> find_equal(const value_type& value) {
        TreeNodePtr p      = root();
        TreeNodePtr* p_ptr = root_ptr();

        if (p == nullptr) {
            return {&end_node_, end_node_.left};
        }

        while (true) {
            if (comp_()(value, p->value())) {
                if (p->left != nullptr) {
                    p_ptr = &(p->left);
                    p     = p->left;

                } else {
                    return {p, p->left};
                }

            } else if (comp_()(p->value(), value)) {
                if (p->right != nullptr) {
                    p_ptr = &(p->right);
                    p     = p->right;

                } else {
                    return {p, p->right};
                }

            } else {
                return {p, *p_ptr};
            }
        }
    }

    void insert_node_at(std::pair<TreeEndNodePtr, TreeNodePtr&> parent_and_child, TreeNodePtr new_node) noexcept {
        TreeEndNodePtr parent = parent_and_child.first;
        TreeNodePtr& child    = parent_and_child.second;

        new_node->left  = nullptr;
        new_node->right = nullptr;
        new_node->set_parent(parent);

        child = new_node;

        if (begin_->left != nullptr) {
            begin_ = begin_->left;
        }

        ciel::balance_after_insert(root(), new_node);
    }

public:
    // Insert node into this tree, return false if the same value exists.
    CIEL_NODISCARD bool insert(TreeNodePtr new_node) noexcept {
        std::pair<TreeEndNodePtr, TreeNodePtr&> parent_and_child = find_equal(new_node->value());

        if (parent_and_child.second != nullptr) {
            return false;
        }

        insert_node_at(parent_and_child, new_node);
        ++size_();
        return true;
    }

    // Remove ptr from this tree, ptr must be valid.
    void remove(TreeNodePtr ptr) noexcept {
        if (begin_ == ptr) {
            begin_ = ciel::next(ptr);
        }

        ciel::remove(root(), ptr);
        --size_();
    }

    // Return the node ptr of value, nullptr if not found.
    CIEL_NODISCARD TreeNodePtr find(const value_type& value) noexcept {
        std::pair<TreeEndNodePtr, TreeNodePtr&> parent_and_child = find_equal(value);
        return parent_and_child.second;
    }

    CIEL_NODISCARD iterator begin() noexcept {
        return {begin_};
    }

    CIEL_NODISCARD iterator end() noexcept {
        return {&end_node_};
    }

    CIEL_NODISCARD size_type size() const noexcept {
        return size_();
    }

    CIEL_NODISCARD bool empty() const noexcept {
        CIEL_ASSERT((size() == 0) == (begin_ == &end_node_));

        return size() == 0;
    }

    CIEL_NODISCARD TreeNodePtr extract_min() noexcept {
        CIEL_ASSERT(!empty());

        TreeNodePtr res = static_cast<TreeNodePtr>(begin_);
        remove(res);
        return res;
    }

}; // class rb_tree

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_RB_TREE_HPP_
