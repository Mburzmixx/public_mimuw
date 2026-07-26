//Author: Mateusz Burza
#include "prev.h"
#include <bits/stdc++.h>

using namespace std;

namespace {
    /*
     * CONTRACT:
     * stores the index of the last term in sequence
     */
    int curr_index = -1;

    /*
     * CLASS "node" CONTAINS:
     * -> index:
     *      max index, such that term with this index has value between left_end and right_end,
     *      index = -1 <=> this node does not exist,
     * -> left, right:
     *      (shared) pointers to children,
     * -> left_end, right_end:
     *      extremes in the considered range of value.
     */
    class node {
        public:
            int index;
            shared_ptr<node> left;
            shared_ptr<node> right;

            int left_end;
            int right_end;

            node() {
                index = -1;
                left_end = right_end = 0;
            }

            node(int Left_end, int Right_end) {
                left_end = Left_end;
                right_end = Right_end;
                index = curr_index;
            }

            bool is_leaf() {
                if (left_end == right_end)  return true;
                return false;
            }

            int mid() {
                // here I fix the problem of dividing interval for ex. [-8,-7].
                if (left_end <= 0 && right_end <= 0)
                    return -1 + left_end / 2 + right_end / 2;
                return left_end / 2 + right_end / 2;
            }

            bool is_on_the_left_side (int value) {
                if (value <= mid())   return true;
                return false;
            }

    };

    /*
     * TYPE max_of_interval IS:
     * a binary tree, containing in each node max value from appropriate interval.
     * It is a persistent pointer-based binary tree.
     * I will create only needed nodes and with each "pushBack", create a new copy of changed path.
     * This means, that I will be able to refer to a past state of this tree.
     */
    typedef shared_ptr<node> max_of_interval;

    /*
     * "roots":
     * a vector of roots, such that i-th element is a pointer to the tree "max_of_interval",
     * is a state just after adding i-th term to the sequence.
     */
    vector<max_of_interval> roots;
}

/*
 * CONTRACT:
 * params:  pointer to a "father" and a bool determining the direction,
 * returns: nothing, creates a relevant son (if left == true, then left ; else right)
 *                          with appropriate interval parameters and index = curr_index
 *                                                              (done in the constructor).
 */
void make_Child(max_of_interval &father, bool left) {
    if (left == true)
        father->left = make_shared<node>(father->left_end, father->mid());
    else
        father->right = make_shared<node>(1 + father->mid(), father->right_end);
}

/*
 * CONTRACT:
 * params:  pointer to a "father" and a bool determining the direction,
 * returns: pointer to relevant son (if left == true, then left ; else right).
 */
max_of_interval get_Child(max_of_interval &father, bool left) {
    if (father == nullptr)
        return nullptr;
    if (left == true)
        return father->left;
    return father->right;
}

/*
 * CONTRACT:
 * params:  pointer to new_node, old_node and a bool determining the direction,
 * returns: nothing, makes an appropriate son of old_node also an appropriate son of new_node.
 */
void link(max_of_interval &new_node, max_of_interval &old_node, bool left) {
    if (left)
        new_node->left = get_Child(old_node, true);
    else
        new_node->right = get_Child(old_node, false);
}

/*
 * CONTRACT:
 * param:  value of an element that will be added at the end of the current sequence,
 * method: with each "pushBack" I will create a new root,
 *              create relevant new branches of interval tree and link the old, unchanged ones.
 */
void pushBack(int value) {
        curr_index++;

    max_of_interval new_root;
    new_root = make_shared<node>(INT_MIN, INT_MAX);

    roots.push_back(new_root);

    max_of_interval new_node = new_root;
    max_of_interval old_node = nullptr;

    if (curr_index != 0)    old_node = roots[curr_index - 1];

    while (new_node != nullptr && !new_node->is_leaf()) {
        bool left = new_node->is_on_the_left_side(value);

        make_Child(new_node, left);
        link(new_node, old_node, !left);

        new_node = get_Child(new_node, left);
        old_node = get_Child(old_node, left);
    }
}

/*
 * CONTRACT:
 * param:   seq vector of integers, representing sequence from the task,
 * returns: nothing, calls the "pushBack" function enough times
 */
void init(const vector<int> &seq) {
    for (auto s : seq) pushBack(s);
}

/*
 * CONTRACT:
 * params:  reference to a node, lower and upper boundaries,
 * returns: in the end returns wanted index for the function "prevInRange",
 * method:  I use recursion typical for segment tree, finding all adequate nodes.
 */
int get_max(max_of_interval &node, int lo, int hi) {
    if (node == nullptr) return -1;

    if (node->right_end < lo || hi < node->left_end) return -1;

    if (lo <= node->left_end && node->right_end <= hi) return node->index;

    return max(get_max(node->left, lo, hi), get_max(node->right, lo, hi));
}

/*
 * CONTRACT:
 * params:  i  - upper bound for the index of a sequence term,
 *          lo - lower bound for the value of a sequence term (not strict),
 *          hi - upper bound for the value of a sequence term (not strict).
 * returns: the largest index j, such that
 *              j = max{ 0 <= k <= j |  l0 <= x_k <= h0} or "-1" if it does not exist.
 */
int prevInRange(int i, int lo, int hi) {
    if (curr_index < i) i = curr_index;

    return get_max(roots[i], lo, hi);
}

/*
 * CONTRACT:
 * frees used space.
 */
void done() {
    roots.clear();
    roots.resize(0);
}