#include <iostream>
#include <vector>
#include <stdexcept>

template <typename T, typename Allocator = std::allocator<T>>
class List {
private:
    struct BasicNode {
        BasicNode* prev;
        BasicNode* next;
        BasicNode() = default;
        BasicNode(BasicNode* _prev, BasicNode* _next) : prev(_prev), next(_next) {}
        BasicNode& operator=(BasicNode&& bn) {
            prev = bn.prev;
            next = bn.next;
            next->prev = prev->next = this;
            return *this;
        }
    };

    struct Node : BasicNode {
        T value;
        Node() = default;
        Node(BasicNode* _prev, BasicNode* _next, const T& _value) : BasicNode(_prev, _next), value(_value) {}
        Node(BasicNode* _prev, BasicNode* _next, T&& _value) : BasicNode(_prev, _next), value(std::move(_value)) {}
        Node(BasicNode* _prev, BasicNode* _next) : BasicNode(_prev, _next) {}
    };

    BasicNode head;
    using AllocTraits = std::allocator_traits<typename std::allocator_traits<Allocator>::template rebind_alloc<Node>>;
    typename AllocTraits::allocator_type alloc;
    Allocator standartAlloc;
    size_t sz = 0;



    void swap(List& lst) {
        std::swap(sz, lst.sz);
        std::swap(head, lst.head);
        std::swap(head.next->prev, lst.head.next->prev);
        std::swap(head.prev->next, lst.head.prev->next);
        std::swap(alloc, lst.alloc);
        std::swap(standartAlloc, lst.standartAlloc);
    }



public:

    template <bool isConst>
    class Iter {
    public:
        BasicNode* ptr;
    public:

        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<isConst, const T, T>;
        using pointer = std::conditional_t<isConst, const T*, T*>;
        using reference = std::conditional_t<isConst, const T&, T&>;
        using iterator_category = std::bidirectional_iterator_tag;


        Iter() : ptr(nullptr) {}

        Iter(const BasicNode* _ptr) : ptr(const_cast<BasicNode*>(_ptr)) {}

        Iter(const Iter<false>& it) : ptr(it.ptr) {}
        Iter(const Iter<true>& it) : ptr(it.ptr) {}

        reference operator*() const {
            return reinterpret_cast<Node*>(ptr)->value;
        }

        pointer operator->() const {
            return &(reinterpret_cast<Node*>(ptr)->value);
        }

        Iter& operator++() {
            ptr = ptr->next;
            return *this;
        }

        Iter operator++(int) {
            Iter new_ptr(ptr);
            ptr = ptr->next;
            return new_ptr;
        }

        Iter& operator--() {
            ptr = ptr->prev;
            return *this;
        }

        Iter operator--(int) {
            Iter new_ptr(ptr);
            ptr = ptr->prev;
            return new_ptr;
        }

        template <bool isEqConst>
        bool operator==(const Iter<isEqConst>& it) const {
            return it.ptr == ptr;
        }

        template <bool isUnEqConst>
        bool operator!=(const Iter<isUnEqConst>& it) const {
            return it.ptr != ptr;
        }

        friend class List;

    };



    using iterator = Iter<false>;
    using const_iterator = Iter<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List(const List& lst) : alloc(AllocTraits::select_on_container_copy_construction(lst.standartAlloc)),
                                              standartAlloc(AllocTraits::select_on_container_copy_construction(lst.standartAlloc)) {
        head.next = head.prev = &head;
        BasicNode* lstBasicNode = lst.head.next;
        size_t i = 0;
        try {
            for (; i < lst.sz; ++i) {
                insert(end(), reinterpret_cast<Node*>(lstBasicNode)->value);
                if (i == lst.sz - 1)
                    break;
                lstBasicNode = lstBasicNode->next;
            }
        } catch (...) {
            for (size_t j = 0; j < i; ++j) {
            	pop_front();
	    }
            throw;
        }
    }

    List(List&& lst) : alloc(std::move(lst.alloc)), standartAlloc(std::move(standartAlloc)), sz(lst.sz) {
        head = std::move(lst.head),
        lst.head.next = lst.head.prev = &lst.head;
        lst.sz = 0;
    }


    List& operator=(List&& lst) {
        while(sz != 0) {
            pop_back();
        }
        head = std::move(lst.head);
        alloc = std::move(lst.alloc);
        standartAlloc = std::move(standartAlloc);
        sz = lst.sz;
        lst.head.next = lst.head.prev = &lst.head;
        lst.sz = 0;
        return *this;
    }


    List(const Allocator& _alloc = Allocator()) : alloc(_alloc), standartAlloc(_alloc) {
        head.next = head.prev = &head;
    }

    List(size_t _sz, const Allocator& _alloc = Allocator()) : sz(_sz), alloc(_alloc), standartAlloc(_alloc) {
	head.next = head.prev = &head;
        size_t i = 0;
        try {
            for (; i < _sz; ++i) {
                BasicNode* cur = reinterpret_cast<BasicNode*>(AllocTraits::allocate(alloc, 1));
                (head.prev)->next = cur;
                AllocTraits::construct(alloc, reinterpret_cast<Node*>(cur), head.prev, &head);
            	head.prev = cur;
            }
        } catch (...) {
            for (size_t j = 0; j < i; ++j) {
                pop_front();
            }
            throw;
        }
    }

    List(size_t _sz, const T& _value, const Allocator& _alloc = Allocator()) : alloc(_alloc), standartAlloc(_alloc) {
        size_t i = 0;
        try {
            for (; i < _sz; ++i) {
                insert(begin(), _value);
            }
        } catch (...) {
            for (size_t j = 0; j < i; ++j) {
            	pop_front();
	    }
            throw;
        }
    }


    List& operator=(const List& lst) {
        if (AllocTraits::propagate_on_container_copy_assignment::value)
            standartAlloc = lst.standartAlloc;
        alloc = standartAlloc;
    	List cpy(alloc);
        for (const_iterator it = lst.begin(); it != lst.end(); ++it) {
	      	cpy.push_back(*it);
        }
	    swap(cpy);
	    return *this;
    }



    size_t size() const {
        return sz;
    }

    Allocator get_allocator() const {
        return standartAlloc;
    }

    void insert(const_iterator it, const T& value) {
        ++sz;
        BasicNode* newBasicNode = reinterpret_cast<BasicNode*>(AllocTraits::allocate(alloc, 1));
        try {
            AllocTraits::construct(alloc, reinterpret_cast<Node*>(newBasicNode), it.ptr->prev, it.ptr, value);
        } catch (...) {
            --sz;
            AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(newBasicNode), 1);
            throw;
        }
        it.ptr->prev->next = newBasicNode;
        it.ptr->prev = newBasicNode;
    }

    void insert(iterator it, T&& value) {
        ++sz;
        BasicNode* newBasicNode = reinterpret_cast<BasicNode*>(AllocTraits::allocate(alloc, 1));
        try {
            AllocTraits::construct(alloc, reinterpret_cast<Node*>(newBasicNode), it.ptr->prev, it.ptr, std::move(value));
        } catch (...) {
            --sz;
            AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(newBasicNode), 1);
            throw;
        }
        it.ptr->prev->next = newBasicNode;
        it.ptr->prev = newBasicNode;
    }




    void erase(const_iterator it) {
        --sz;
        it.ptr->next->prev = it.ptr->prev;
        it.ptr->prev->next = it.ptr->next;
        AllocTraits::destroy(alloc, reinterpret_cast<Node*>(it.ptr));
        AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(it.ptr), 1);
    }

    void push_back(const T& value) {
        insert(end(), value);
    }

    void push_front(const T& value) {
        insert(begin(), value);
    }


    void pop_front() {
        erase(begin());
    }

    void pop_back() {
        erase(--end());
    }

    iterator begin() {
        return iterator(head.next);
    }

    const_iterator begin() const {
        return const_iterator(head.next);
    }

    const_iterator cbegin() const {
        return const_iterator(head.next);
    }

    iterator end() {
        return iterator(&head);
    }

    const_iterator end() const {
        return const_iterator(&head);
    }

    const_iterator cend() const {
        return const_iterator(&head);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(&head);
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(&head);
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(&head);
    }

    reverse_iterator rend() {
        return reverse_iterator(head.next);
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(head.next);
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(head.next);
    }

    ~List() {
    	while(sz != 0) {
            pop_back();
        }
    }

};



template <typename Key, typename Value,
            typename Hash = std::hash<Key>,
            typename Equal = std::equal_to<Key>,
            typename Alloc = std::allocator< std::pair<const Key, Value>>>
class UnorderedMap {
public:
    using NodeType = std::pair<const Key, Value>;

private:

    struct Node {
        NodeType value;
        size_t cached_hash;
        Node(const NodeType& _value, size_t _cached_hash) : value(_value), cached_hash(_cached_hash) {}
        Node(const Node& nd) : value(nd.value), cached_hash(nd.cached_hash) {}
        Node(Key&& _key, Value&& _value, size_t _cached_hash) : value(std::move(_key), std::move(_value)), cached_hash(_cached_hash) {}
        Node(Key&& _key, Value&& _value) : value(std::move(_key), std::move(_value)) {}
        Node(const Key& _key, const Value& _value) : value(_key, _value) {}
        Node(Node&& nd) : value(std::move(const_cast<Key&>(nd.value.first)), std::move(nd.value.second)), cached_hash(nd.cached_hash) {}
    };

    using AllocTraits = std::allocator_traits<typename std::allocator_traits<Alloc>::template rebind_alloc<Node>>;

    List<Node, typename AllocTraits::allocator_type> elements;
    std::vector<typename List<Node, typename AllocTraits::allocator_type>::iterator> nodes;
    size_t sz = 0;
    int loadfactor;


public:


    template <bool isConst>
    class Iter {
    private:
        using T = std::conditional_t<isConst, typename List<Node, typename AllocTraits::allocator_type>::const_iterator,
                                              typename List<Node, typename AllocTraits::allocator_type>::iterator>;
        T ptr;
    public:

        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<isConst, const NodeType, NodeType>;
        using pointer = std::conditional_t<isConst, const NodeType*, NodeType*>;
        using reference = std::conditional_t<isConst, const NodeType&, NodeType&>;
        using iterator_category = std::forward_iterator_tag;

        Iter(const T& _ptr) : ptr(_ptr) {}

        Iter(const Iter<false>& it) : ptr(it.ptr) {}

        reference operator*() const {
            return (*ptr).value;
        }

        pointer operator->() const {
            return &((*ptr).value);
        }

        Iter& operator++() {
            ++ptr;
            return *this;
        }

        Iter& operator--() {
            --ptr;
            return *this;
        }

        Iter operator++(int) {
            T new_ptr(ptr);
            ++ptr;
            return new_ptr;
        }

        template <bool isEqConst>
        bool operator==(const Iter<isEqConst>& it) const {
            return it.ptr == ptr;
        }

        template <bool isUnEqConst>
        bool operator!=(const Iter<isUnEqConst>& it) const {
            return it.ptr != ptr;
        }

	friend class UnorderedMap;
    };

    using iterator = Iter<false>;
    using const_iterator = Iter<true>;


    UnorderedMap() {
        nodes.resize(100000);
    }


    UnorderedMap& operator=(UnorderedMap&& um) {
        nodes = std::move(um.nodes);
        elements = std::move(um.elements);
        sz = um.sz;
        return *this;
    }


    UnorderedMap(const UnorderedMap& um) : elements(um.elements), nodes(um.nodes), sz(um.sz) {}

    UnorderedMap(UnorderedMap&& um) : elements(std::move(um.elements)), nodes(std::move(um.nodes)), sz(um.sz) {}


    void clear() {
        nodes.clear();
        while(elements.size() != 0) {
            elements.pop_back();
        }
    }

    Value& operator[](const Key& key) {
        size_t mod_hash = Hash{}(key) % nodes.size();
        size_t _hash = Hash{}(key);
        if (nodes[mod_hash] == typename List<Node, typename AllocTraits::allocator_type>::iterator(nullptr)) {
            try {
                elements.insert(elements.begin(), {{key, Value()}, _hash});
                nodes[mod_hash] = elements.begin();
                ++sz;
                return elements.begin()->value.second;
            } catch (...) {
               throw;
            }
        }
        for (auto it = nodes[mod_hash]; it != elements.end(); ++it) {
            if ((*it).cached_hash % nodes.size() != mod_hash) {
                break;
            }
            if (Equal{}(it->value.first, key)) {
                return it->value.second;
            }
        }
        try {
            elements.insert(elements.begin(), {{key, Value()}, _hash});
            nodes[mod_hash] = elements.begin();
            ++sz;
            return elements.begin()->value.second;
        } catch (...) {
           throw;
        }
    }


    Value& at(const Key& key) {
        size_t mod_hash = Hash{}(key) % nodes.size();
        if (nodes[mod_hash] == typename List<Node, typename AllocTraits::allocator_type>::iterator(nullptr))
            throw std::out_of_range("mlg");
        for (auto it = nodes[mod_hash]; it != elements.end(); ++it) {
            if ((*it).cached_hash % nodes.size() != mod_hash) {
                throw std::out_of_range("mlg");
            }
            if (Equal{}(it->value.first, key))
                return it->value.second;
        }
        throw std::out_of_range("mlg");
    }

    const Value& at(const Key& key) const {
        size_t mod_hash = Hash{}(key) % nodes.size();
        size_t _hash = Hash{}(key);
        if (nodes[mod_hash] == typename List<Node, typename AllocTraits::allocator_type>::iterator(nullptr))
            throw std::out_of_range("mlg");
        for (auto it = nodes[mod_hash]; it != elements.end(); ++it) {
            if ((*it).cached_hash % nodes.size() != mod_hash) {
                throw std::out_of_range("mlg");
            }
            if (Equal{}(it->value.first, key))
                return it->value.second;
        }
        throw std::out_of_range();
    }

    size_t size() const {
        return sz;
    }

    iterator begin() {
        return iterator(elements.begin());
    }

    const_iterator begin() const {
        return const_iterator(elements.begin());
    }

    const_iterator cbegin() const {
        return const_iterator(elements.begin());
    }

    iterator end() {
        return iterator(elements.end());
    }

    const_iterator end() const {
        return const_iterator(elements.end());
    }

    const_iterator cend() const {
        return const_iterator(elements.end());
    }


    iterator find(const Key& key) {
        size_t mod_hash = Hash{}(key) % nodes.size();
        if (nodes[mod_hash] == typename List<Node, typename AllocTraits::allocator_type>::iterator(nullptr))
            return end();
        for (auto it = nodes[mod_hash]; it != elements.end(); ++it) {
            if ((*it).cached_hash % nodes.size() != mod_hash)
                return end();
            if (Equal{}(it->value.first, key))
                return iterator(it);
        }
        return end();
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }


    void erase(const_iterator it) {
        size_t _hash = (*(it.ptr)).cached_hash;
        size_t mod_hash = _hash % nodes.size();
        if (nodes[mod_hash] == it.ptr) {
            auto it1 = it;
            ++it1;
            if ((*(it1.ptr)).cached_hash == mod_hash)
                nodes[mod_hash] = it1.ptr;
            else
                nodes[mod_hash] = nullptr;
        }
        elements.erase(it.ptr);
        --sz;
    }

    void erase(const_iterator first, const_iterator last) {
        ++first;
        for (auto it = first; it != last; ++it) {
            auto it1 = it;
            --it1;
            erase(it1);
        }
        erase(--last);
    }

    template <typename... Args>
    std::pair <iterator, bool> emplace(Args&&... args) {
        typename AllocTraits::allocator_type alloc = elements.get_allocator();
        Node* _node = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, _node, std::forward<Args>(args)...);
        NodeType nodetype(std::move(const_cast<Key&>(_node->value.first)), std::move(_node->value.second));
        Node node(std::move(const_cast<Key&>(nodetype.first)), std::move(nodetype.second), Hash{}(nodetype.first));
        AllocTraits::deallocate(alloc, _node, 1);
        size_t mod_hash = node.cached_hash % nodes.size();
        if (nodes[mod_hash] == typename List<Node, typename AllocTraits::allocator_type>::iterator(nullptr)) {
            elements.insert(elements.begin(), std::move(node));
            nodes[mod_hash] = elements.begin();
            ++sz;
            return {iterator(elements.begin()), true};
        }
        for (auto it = nodes[mod_hash]; it != elements.end(); ++it) {
            if ((*it).cached_hash % nodes.size() != mod_hash) {
                try {
                    elements.insert(it, std::move(node));
                    ++sz;
                    --it;
                    return {iterator(it), true};
                } catch (...) {
                    throw;
                }
            }
            if (Equal{}(it->value.first, node.value.first))
                return {iterator(it), false};
        }
        try {
            elements.insert((--end()).ptr, std::move(node));
            ++sz;
            return {--end(), true};
        } catch (...) {
            throw;
        }
    }

    std::pair<iterator, bool> insert(const NodeType& value) {
        return emplace(const_cast<Key&>(value.first), value.second);
    }

    std::pair<iterator, bool> insert(NodeType&& value) {
        return emplace(std::move(const_cast<Key&>(value.first)), std::move(value.second));
    }


    void reserve(size_t) {}
};




