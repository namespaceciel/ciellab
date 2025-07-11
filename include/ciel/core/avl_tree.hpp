#ifndef CIELLAB_INCLUDE_CIEL_CORE_AVL_TREE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_AVL_TREE_HPP_

#include <ciel/core/compressed_pair.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>
#include <ciel/core/packed_ptr.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class Derived>
struct avl_node_base {
    using pointer = Derived*;

    pointer left{nullptr};

}; // struct avl_node_base

template<class T>
struct avl_node : avl_node_base<avl_node<T>> {
    using base_type    = avl_node_base<avl_node<T>>;
    using base_pointer = base_type*;
    using pointer      = avl_node*;

    using base_type::left;
    pointer right{nullptr};

private:
    // Given AVL's max_size 2^64-1, one byte is enough to hold height.
    packed_ptr<base_type> parent_height_{nullptr, 1};

public:
    T value;

    template<class... Args>
    avl_node(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args&&...>::value)
        : value(std::forward<Args>(args)...) {}

    CIEL_NODISCARD uint8_t height() const noexcept {
        return parent_height_.count();
    }

    void set_height(uint8_t h) noexcept {
        return parent_height_.set_count(h);
    }

    CIEL_NODISCARD base_pointer parent() const noexcept {
        return parent_height_.ptr();
    }

    void set_parent(base_pointer p) noexcept {
        return parent_height_.set_ptr(p);
    }

    CIEL_NODISCARD pointer parent_unsafe() const noexcept {
        return static_cast<pointer>(parent());
    }

    CIEL_NODISCARD bool is_left_child() const noexcept {
        return parent()->left == this;
    }

    CIEL_NODISCARD static uint8_t get_height(pointer node) noexcept {
        return node ? node->height() : 0;
    }

    void adjust_height() noexcept {
        set_height(1 + ciel::max(get_height(left), get_height(right)));
    }

}; // struct avl_node

template<class, class>
class avl_tree;

template<class T, class Pointer, class Reference>
class avl_iterator {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = Pointer;
    using reference         = Reference;
    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept  = std::bidirectional_iterator_tag;

private:
    using node_type      = avl_node<value_type>;
    using base_node_type = avl_node_base<node_type>;

    base_node_type* it_;

    template<class, class>
    friend class avl_tree;

public:
    avl_iterator() noexcept
        : it_(nullptr) {}

    avl_iterator(const base_node_type* p) noexcept
        : it_(const_cast<base_node_type*>(p)) {}

    template<class P, class R>
    avl_iterator(const avl_iterator<T, P, R>& other) noexcept
        : it_(const_cast<base_node_type*>(other.cast())) {}

    avl_iterator(const avl_iterator&)            = default;
    avl_iterator& operator=(const avl_iterator&) = default;

    CIEL_NODISCARD base_node_type* cast() const noexcept {
        return it_;
    }

    CIEL_NODISCARD node_type* downcast_unsafe() const noexcept {
        return static_cast<node_type*>(it_);
    }

private:
    CIEL_NODISCARD bool is_left_child() const noexcept {
        return downcast_unsafe()->is_left_child();
    }

    void goto_tree_min() noexcept {
        while (left()) {
            *this = left();
        }
    }

    void goto_tree_max() noexcept {
        while (right()) {
            *this = right();
        }
    }

public:
    CIEL_NODISCARD reference operator*() const noexcept {
        return downcast_unsafe()->value;
    }

    CIEL_NODISCARD pointer operator->() const noexcept {
        return &downcast_unsafe()->value;
    }

    avl_iterator& operator++() noexcept {
        auto& self = *this;

        if (right()) {
            self = right();
            goto_tree_min();

        } else {
            while (!is_left_child()) {
                self = parent();
            }

            self = parent();
        }

        return self;
    }

    avl_iterator& operator--() noexcept {
        auto& self = *this;

        if (left()) {
            self = left();
            goto_tree_max();

        } else {
            while (is_left_child()) {
                self = parent();
            }

            self = parent();
        }

        return self;
    }

    CIEL_NODISCARD avl_iterator operator++(int) noexcept {
        avl_iterator res(*this);
        ++(*this);
        return res;
    }

    CIEL_NODISCARD avl_iterator operator--(int) noexcept {
        avl_iterator res(*this);
        --(*this);
        return res;
    }

    CIEL_NODISCARD avl_iterator next() const noexcept {
        avl_iterator res(*this);
        ++res;
        return res;
    }

    CIEL_NODISCARD avl_iterator prev() const noexcept {
        avl_iterator res(*this);
        --res;
        return res;
    }

    CIEL_NODISCARD avl_iterator left() const noexcept {
        return {it_->left};
    }

    CIEL_NODISCARD avl_iterator right() const noexcept {
        return {downcast_unsafe()->right};
    }

    CIEL_NODISCARD avl_iterator parent() const noexcept {
        return {downcast_unsafe()->parent()};
    }

    CIEL_NODISCARD explicit operator bool() const noexcept {
        return it_ != nullptr;
    }

}; // class avl_iterator

template<class T, class Pointer1, class Pointer2, class Reference1, class Reference2>
CIEL_NODISCARD bool operator==(const avl_iterator<T, Pointer1, Reference1>& lhs,
                               const avl_iterator<T, Pointer2, Reference2>& rhs) noexcept {
    return lhs.cast() == rhs.cast();
}

template<class T, class Pointer1, class Pointer2, class Reference1, class Reference2>
CIEL_NODISCARD bool operator!=(const avl_iterator<T, Pointer1, Reference1>& lhs,
                               const avl_iterator<T, Pointer2, Reference2>& rhs) noexcept {
    return !(lhs == rhs);
}

template<class T, class Compare>
class avl_tree {
public:
    using value_type             = T;
    using value_compare          = Compare;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = avl_iterator<value_type, pointer, reference>;
    using const_iterator         = avl_iterator<value_type, const_pointer, const_reference>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    using node_type      = avl_node<value_type>;
    using base_node_type = avl_node_base<node_type>;

    base_node_type* start_{&end_node_};
    base_node_type end_node_;
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

    CIEL_NODISCARD static uint8_t get_height(node_type* node) noexcept {
        return node_type::get_height(node);
    }

    void right_rotate(node_type* head) noexcept {
        /**
         *         head          new_head
         *        /    \           /     \
         *   new_head   Z   -->   X     head
         *    /    \                    /   \
         *   X     Y                   Y     Z
         */
        node_type* new_head = head->left;
        head->left          = new_head->right;
        new_head->right     = head;

        if (head->is_left_child()) {
            head->parent()->left = new_head;

        } else {
            head->parent_unsafe()->right = new_head;
        }

        new_head->set_parent(head->parent());
        head->set_parent(new_head);

        if (head->left) {
            head->left->set_parent(head);
        }

        head->adjust_height();
        new_head->adjust_height();
    }

    void left_rotate(node_type* head) noexcept {
        /**
         *      head                  new_head
         *    /     \                 /     \
         *   X     new_head   -->   head     Z
         *         /   \           /   \
         *        Y     Z         X     Y
         */
        node_type* new_head = head->right;
        head->right         = new_head->left;
        new_head->left      = head;

        if (head->is_left_child()) {
            head->parent()->left = new_head;

        } else {
            head->parent_unsafe()->right = new_head;
        }

        new_head->set_parent(head->parent());
        head->set_parent(new_head);

        if (head->right) {
            head->right->set_parent(head);
        }

        head->adjust_height();
        new_head->adjust_height();
    }

public:
    iterator insert_node_multi(node_type* new_node) noexcept {
        // lower/upper_bound would both work but lower_bound may increase the chances updating start_.
        // e.g. All nodes contain the same value and start_ updates everytime.
        return insert_node_before(upper_bound(new_node->value), new_node);
    }

    std::pair<iterator, bool> insert_node_unique(node_type* new_node) noexcept {
        auto lb = lower_bound(new_node->value);              // key <= *lb

        if (lb != end() && !comp_()(new_node->value, *lb)) { // *lb <= key
            return {lb, false};
        }

        return {insert_node_before(lb, new_node), true};
    }

    // Unconditionally insert new_node before pos.
    iterator insert_node_before(iterator pos, node_type* new_node) noexcept {
        const iterator res{[&] {
            if (pos.left()) {
                iterator l = pos.left();
                l.goto_tree_max();
                l.downcast_unsafe()->right = new_node;
                new_node->set_parent(l.cast());
                return l.right();
            }

            pos.cast()->left = new_node;
            new_node->set_parent(pos.cast());

            if (pos.cast() == start_) {
                start_ = new_node;
            }

            return pos.left();
        }()};

        ++size_();

        // Adjust from bottom to top.
        node_type* upping = res.downcast_unsafe();
        while (upping->parent() != &end_node_) {
            upping = upping->parent_unsafe();
            upping->adjust_height();

            const auto balance =
                ciel::signed_cast(get_height(upping->left)) - ciel::signed_cast(get_height(upping->right));
            // Following codes will only happen once at most.
            if CIEL_UNLIKELY (balance > 1) {
                if (comp_()(new_node->value, upping->left->value)) {
                    right_rotate(upping);

                } else {
                    left_rotate(upping->left);
                    right_rotate(upping);
                }

                upping = upping->parent_unsafe();

            } else if CIEL_UNLIKELY (balance < -1) {
                if (comp_()(upping->right->value, new_node->value)) {
                    left_rotate(upping);

                } else {
                    right_rotate(upping->right);
                    left_rotate(upping);
                }

                upping = upping->parent_unsafe();
            }
        }

        return res;
    }

    CIEL_NODISCARD node_type* pop_node(const iterator node) noexcept {
        CIEL_ASSERT(node != end());

        if (node == begin()) {
            start_ = node.next().cast();
        }

        // upping records the first position that needs to recalculate height,
        // when deletion is done, go up taking care of height and balance from upping.
        // Note: upping is always node.parent() except for node having both children.
        base_node_type* upping = node.parent().cast();

        if (node.left()) {
            // If node has both children, find previous node to replace it. upping will update only in this situation.
            if (node.right()) {
                node_type* prev = node.prev().downcast_unsafe();
                if (prev->is_left_child()) {
                    // If prev is left child, then node.left() == prev and prev doesn't have right child.
                    /**
                     *        parent               parent
                     *          |                    |
                     *         node                 prev
                     *        /    \               /    \
                     *      prev   right   -->   ...    right
                     *     /
                     *   ...
                     */
                    upping = prev;

                    prev->right = node.right().downcast_unsafe();
                    node.right().downcast_unsafe()->set_parent(prev);

                    if (node.is_left_child()) {
                        node.parent().cast()->left = prev;

                    } else {
                        node.parent().downcast_unsafe()->right = prev;
                    }

                    prev->set_parent(node.parent().cast());

                } else {
                    // prev still doesn't have right child.
                    /**
                     *        parent                  parent
                     *          |                       |
                     *         node                    prev
                     *        /    \                  /    \
                     *      left   right    -->    left   right
                     *     /    \                 /    \
                     *   ...    ...             ...    ...
                     *            \                      \
                     *        prev_parent            prev_parent
                     *             \                      \
                     *            prev                   ...
                     *            /
                     *          ...
                     */
                    upping = prev->parent();

                    prev->parent_unsafe()->right = prev->left;
                    if (prev->left) {
                        prev->left->set_parent(prev->parent());
                    }

                    prev->left  = node.left().downcast_unsafe();
                    prev->right = node.right().downcast_unsafe();
                    prev->set_parent(node.parent().cast());

                    prev->left->set_parent(prev);
                    prev->right->set_parent(prev);

                    if (node.is_left_child()) {
                        prev->parent()->left = prev;

                    } else {
                        prev->parent_unsafe()->right = prev;
                    }
                }

            } else { // only has left child
                if (node.is_left_child()) {
                    /**
                     *        parent
                     *       /     \
                     *     node    ...
                     *     /
                     *   left
                     */
                    node.parent().cast()->left = node.left().downcast_unsafe();

                } else {
                    /**
                     *      parent
                     *     /     \
                     *   ...    node
                     *          /
                     *        left
                     */
                    node.parent().downcast_unsafe()->right = node.left().downcast_unsafe();
                }

                node.left().downcast_unsafe()->set_parent(node.parent().cast());
            }

        } else if (node.right()) { // only has right child
            if (node.is_left_child()) {
                /**
                 *      parent
                 *     /     \
                 *   node    ...
                 *     \
                 *   right
                 */
                node.parent().cast()->left = node.right().downcast_unsafe();

            } else {
                /**
                 *      parent
                 *     /     \
                 *   ...    node
                 *            \
                 *           right
                 */
                node.parent().downcast_unsafe()->right = node.right().downcast_unsafe();
            }

            node.right().downcast_unsafe()->set_parent(node.parent().cast());

        } else { // leaf node
            if (node.is_left_child()) {
                node.parent().cast()->left = nullptr;

            } else {
                node.parent().downcast_unsafe()->right = nullptr;
            }
        }

        while (upping != &end_node_) {
            node_type* up = static_cast<node_type*>(upping);
            up->adjust_height();

            const auto balance = ciel::signed_cast(get_height(up->left)) - ciel::signed_cast(get_height(up->right));
            if CIEL_UNLIKELY (balance > 1) {
                if (get_height(up->left->left) >= get_height(up->left->right)) {
                    right_rotate(up);

                } else {
                    left_rotate(up->left);
                    right_rotate(up);
                }

                up = up->parent_unsafe();

            } else if CIEL_UNLIKELY (balance < -1) {
                if (get_height(up->right->right) >= get_height(up->right->left)) {
                    left_rotate(up);

                } else {
                    right_rotate(up->right);
                    left_rotate(up);
                }

                up = up->parent_unsafe();
            }

            upping = up->parent();
        }

        --size_();
        return node.downcast_unsafe();
    }

    CIEL_NODISCARD iterator begin() noexcept {
        return {start_};
    }

    CIEL_NODISCARD const_iterator begin() const noexcept {
        return {start_};
    }

    CIEL_NODISCARD iterator end() noexcept {
        return {&end_node_};
    }

    CIEL_NODISCARD const_iterator end() const noexcept {
        return {&end_node_};
    }

    template<class Key>
    CIEL_NODISCARD const_iterator lower_bound(const Key& key) const noexcept {
        iterator res(end());
        iterator root(end().left());

        while (root) {
            if (!comp_()(*root, key)) {
                res  = root;
                root = root.left();

            } else {
                root = root.right();
            }
        }

        return res;
    }

    template<class Key>
    CIEL_NODISCARD const_iterator upper_bound(const Key& key) const noexcept {
        iterator res(end());
        iterator root(end().left());

        while (root) {
            if (comp_()(key, *root)) {
                res  = root;
                root = root.left();

            } else {
                root = root.right();
            }
        }

        return res;
    }

    template<class Key>
    CIEL_NODISCARD const_iterator find(const Key& key) const noexcept {
        auto lb = lower_bound(key);              // key <= *lb

        if (lb != end() && !comp_()(key, *lb)) { // *lb <= key
            return lb;
        }

        return end();
    }

    template<class Key>
    CIEL_NODISCARD bool contains(const Key& key) const noexcept {
        return find(key) != end();
    }

    CIEL_NODISCARD size_type size() const noexcept {
        return size_();
    }

    CIEL_NODISCARD bool empty() const noexcept {
        return size() == 0;
    }

}; // class avl_tree

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_AVL_TREE_HPP_
