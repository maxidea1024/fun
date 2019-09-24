//TODO 커스텀 할당자를 지원할 수 있도록 해야함!
//TODO 값 저장부분도 TypeCompatibleStorage로 처리하는게 좋을듯..?

#pragma once

#include "fun/base/base.h"
#include "fun/base/serialization/archive.h"
#include <initializer_list>

namespace fun {

//
// List<ElementType>
//

// 풀링등을 통한 node 할당 처리는 하지 않는다. 지저분하다.
// 자체적으로 전역 메모리 풀링을 하므로, 깔끔한 코드를 유지한채 작업 하도록 하자.
//
// 그냥 allocator를 지정할 수 있도록 하는게 좋을듯...

//TODO sorting을 지원하도록 하자.

template <typename _ElementType>
class List {
 public:
  using ElementType = _ElementType;

 protected:
  class Node {
   public:
    Node(const ElementType& value, Node* next, Node* prev)
      : value_(value), next_(next), prev_(prev) {}

    Node(ElementType&& value, Node* next, Node* prev)
      : value_(MoveTemp(value)), next_(next_), prev_(prev_) {}

    Node* next_;
    Node* prev_;
    ElementType value_;

   public:
    const ElementType& Value() const {
      return value_;
    }

    ElementType& Value() {
      return value_;
    }

    const Node* Next() const {
      return next_;
    }

    Node* Next() {
      return next_;
    }

    const Node* Prev() const {
      return prev_;
    }

    Node* Prev() {
      return prev_;
    }
  };

 public:
  class Iterator {
   public:
    Iterator(Node* node, bool reversed = false)
      : current_node_(node)
      , reversed_(reversed)
      , element_has_been_removed_(false) {}

    //Iterator(const Iterator&) = default;
    //Iterator& operator = (const Iterator&) = default;
    //Iterator& operator = (Iterator&&) = default;

    Iterator(const Iterator& rhs)
      : current_node_(rhs.current_node_)
      , reversed_(rhs.reversed_)
      , element_has_been_removed_(false) {}

    Iterator& operator = (const Iterator& rhs) {
      if (&rhs != this) {
        current_node_ = rhs.current_node_;
        reversed_ = rhs.reversed_;
        element_has_been_removed_ = false;
      }

      return *this;
    }

    bool IsValid() const {
      return !!current_node_;
    }

    explicit operator bool() const {
      return IsValid();
    }

    bool operator !() const {
      return IsValid() == false;
    }

    Iterator& operator ++ () {
      if (!element_has_been_removed_) {
        fun_check_ptr(current_node_);
        current_node_ = reversed_ ? current_node_->prev_ : current_node_->next_;
      } else {
        element_has_been_removed_ = false;
      }

      return *this;
    }

    Iterator operator ++ (int) {
      auto tmp = *this;
      ++(*this);

      return tmp;
    }

    Iterator& operator -- () {
      if (!element_has_been_removed_) {
        fun_check_ptr(current_node_);
        current_node_ = reversed_ ? current_node_->next_ : current_node_->prev_;
      } else {
        element_has_been_removed_ = false;
      }

      return *this;
    }

    Iterator operator -- (int) {
      auto tmp = *this;
      --(*this);

      return tmp;
    }

    ElementType& operator -> () const {
      fun_check_ptr(current_node_);
      return current_node_->value_;
    }

    ElementType& operator * () const {
      fun_check_ptr(current_node_);
      return current_node_->value_;
    }

    ElementType& Value() {
      fun_check_ptr(current_node_);
      return current_node_->value_;
    }

    Node* GetNode() const {
      return current_node_;
    }

    bool operator == (const Iterator& rhs) const {
      return current_node_ == rhs.current_node_;
    }

    bool operator != (const Iterator& rhs) const {
      return current_node_ != rhs.current_node_;
    }

    //void RemoveCurrent() {
    //  fun_check(element_has_been_removed_ == false); // assert duplicated remove.
    //}

   private:
    friend class List<ElementType>;

    Node* current_node_;
    bool reversed_;
    bool element_has_been_removed_;
  };

  struct ReversedIterator : public Iterator {
    ReversedIterator(Node* starting_node)
      : Iterator(starting_node, true) {}
  };

  struct ConstIterator {
    ConstIterator(const Node* node, bool reversed = false)
      : current_node_(node)
      , reversed_(reversed) {}

    ConstIterator(const ConstIterator&) = default;
    ConstIterator& operator = (const ConstIterator&) = default;
    ConstIterator& operator = (ConstIterator&&) = default;

    bool IsValid() const {
      return !!current_node_;
    }

    explicit operator bool() const {
      return IsValid();
    }

    ConstIterator& operator ++ () {
      fun_check_ptr(current_node_);
      current_node_ = reversed_ ? current_node_->prev_ : current_node_->next_;
      return *this;
    }

    ConstIterator operator ++ (int) {
      auto tmp = *this; ++(*this);
      return tmp;
    }

    ConstIterator& operator -- () {
      fun_check_ptr(current_node_);
      current_node_ = reversed_ ? current_node_->next_ : current_node_->prev_;
      return *this;
    }

    ConstIterator operator -- (int) {
      auto tmp = *this; --(*this);
      return tmp;
    }

    const ElementType& operator -> () const {
      fun_check_ptr(current_node_);
      return current_node_->value_;
    }

    const ElementType& operator * () const {
      fun_check_ptr(current_node_);
      return current_node_->value_;
    }

    const ElementType& Value() const {
      fun_check_ptr(current_node_);
      return current_node_->value_;
    }

    const Node* GetNode() const {
      return current_node_;
    }

    bool operator == (const ConstIterator& rhs) const {
      return current_node_ == rhs.current_node_;
    }

    bool operator != (const ConstIterator& rhs) const {
      return current_node_ != rhs.current_node_;
    }

   private:
    friend class List<ElementType>;

    const Node* current_node_;
    bool reversed_;
  };

  struct ReversedConstIterator : public ConstIterator {
    ReversedConstIterator(const Node* starting_node)
      : ConstIterator(starting_node, true) {}
  };

 public:
  List() : head_(nullptr), tail_(nullptr), count_(0) {}

  List(std::initializer_list<ElementType> init_list)
    : head_(nullptr), tail_(nullptr), count_(0) {
    for (const auto& item : init_list) {
      Append(item);
    }
  }

  List(const List& rhs)
    : head_(nullptr), tail_(nullptr), count_(0) {
    for (const Node* node = rhs.head_; node; node = node->next_) {
      Append(node->value_);
    }
  }

  List& operator = (std::initializer_list<ElementType> init_list) {
    Clear();

    for (const auto& item : init_list) {
      Append(item);
    }

    return *this;
  }

  List& operator = (const List& rhs) {
    Clear();

    for (const Node* node = rhs.head_; node; node = node->next_) {
      Append(node->value_);
    }

    return *this;
  }

  List& operator = (List&& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      head_ = rhs.head_;
      tail_ = rhs.tail_;
      count_ = rhs.count_;

      rhs.head_ = nullptr;
      rhs.tail_ = nullptr;
      rhs.count_ = 0;
    }

    return *this;
  }

  ~List() {
    Clear();
  }


  //
  // Prepends
  //

 public:
  void Prepend(const ElementType& item) {
    Node* new_node = new Node(item, head_, nullptr);
    if (head_) {
      head_->prev_ = new_node;
    } else {
      tail_ = new_node;
    }
    head_ = new_node;

    ++count_;
  }

  void Prepend(ElementType&& item) {
    Node* new_node = new Node(MoveTemp(item), head_, nullptr);
    if (head_) {
      head_->prev_ = new_node;
    } else {
      tail_ = new_node;
    }
    head_ = new_node;

    ++count_;
  }

  ElementType& PrependDefault() {
    Prepend(ElementType());
    return Back();
  }

  void Prepend(const ElementType& item, int32 item_count) {
    for (int32 i = 0; i < item_count; ++i) {
      Prepend(item);
    }
  }

  void Prepend(const List& list) {
    // reversed order.
    for (const Node* node = list.tail_; node; node = node->prev_) {
      Prepend(node->value_);
    }
  }

  void Prepend(std::initializer_list<ElementType> init_list) {
    // reversed order.
    if (FUN_LIKELY(init_list.size())) {
      const int32 src_count = int32(init_list.size());
      const ElementType* src = init_list.end();
      for (int32 i = 0; i < src_count; ++i) {
        Prepend(src[-1]);
        --src;
      }
    }
  }


  //
  // Appends
  //

 public:
  void Append(const ElementType& item) {
    Node* new_node = new Node(item, nullptr, tail_);
    if (tail_) {
      tail_->next_ = new_node;
    } else {
      head_ = new_node;
    }
    tail_ = new_node;

    ++count_;
  }

  void Append(ElementType&& item) {
    Node* new_node = new Node(MoveTemp(item), nullptr, tail_);
    if (tail_) {
      tail_->next_ = new_node;
    } else {
      head_ = new_node;
    }
    tail_ = new_node;

    ++count_;
  }

  ElementType& AppendDefault() {
    Append(ElementType());
    return Front();
  }

  void Append(const ElementType& item, int32 item_count) {
    for (int32 i = 0; i < item_count; ++i) {
      Append(item);
    }
  }

  void Append(const List& list) {
    for (const Node* node = list.head_; node; node = node->next_) {
      Append(node->value_);
    }
  }

  void Append(std::initializer_list<ElementType> init_list) {
    for (const ElementType& item : init_list) {
      Append(item);
    }
  }

  List& operator << (const ElementType& item) {
    Append(item);
    return *this;
  }

  List& operator << (ElementType&& item) {
    Append(MoveTemp(item));
    return *this;
  }

  List& operator << (const List& list) {
    Append(list);
    return *this;
  }

  //List& operator << (std::initializer_list<ElementType> init_list) {
  //  Append(init_list);
  //  return *this;
  //}

  List& operator += (const ElementType& item) {
    Append(item);
    return *this;
  }

  List& operator += (ElementType&& item) {
    Append(MoveTemp(item));
    return *this;
  }

  List& operator += (const List& list) {
    Append(list);
    return *this;
  }

  //List& operator += (std::initializer_list<ElementType> init_list) {
  //  Append(init_list);
  //  return *this;
  //}


  //
  // Insertions
  //

 public:
  void InsertFront(const ElementType& item) {
    Prepend(item);
  }

  void InsertFront(ElementType&& item) {
    Prepend(MoveTemp(item));
  }

  void InsertBefore(Iterator& position, const ElementType& item) {
    fun_check(position.IsValid());
    Node* pos_node = position.current_node_;
    Node* pos_next = pos_node;
    Node* pos_prev = pos_node->prev_;
    Node* new_node = new Node(item, pos_next, pos_prev);
    if (pos_prev == nullptr) { // this is the first element.
      head_ = new_node;
    } else {
      pos_node->prev_->next_ = new_node;
    }
    pos_node->prev_ = new_node;

    ++count_;
  }

  void InsertBefore(Iterator& position, ElementType&& item) {
    fun_check(position.IsValid());
    Node* pos_node = position.current_node_;
    Node* pos_next = pos_node;
    Node* pos_prev = pos_node->prev_;
    Node* new_node = new Node(MoveTemp(item), pos_next, pos_prev);
    if (pos_prev == nullptr) { // this is the first element.
      head_ = new_node;
    } else {
      pos_node->prev_->next_ = new_node;
    }
    pos_node->prev_ = new_node;

    ++count_;
  }

  void InsertAfter(Iterator& position, const ElementType& item) {
    fun_check(position.IsValid());
    Node* pos_node = position.current_node_;
    Node* pos_next = pos_node->next_;
    Node* pos_prev = pos_node;
    Node* new_node = new Node(item, pos_next, pos_prev);
    if (pos_next == nullptr) { // this is the last element
      tail_ = new_node;
    } else {
      pos_node->next_->prev_ = new_node;
    }
    pos_node->next_ = new_node;

    ++count_;
  }

  void InsertAfter(Iterator& position, ElementType&& item) {
    fun_check(position.IsValid());
    Node* pos_node = position.current_node_;
    Node* pos_next = pos_node->next_;
    Node* pos_prev = pos_node;
    Node* new_node = new Node(MoveTemp(item), pos_next, pos_prev);
    if (pos_next == nullptr) { // this is the last element
      tail_ = new_node;
    } else {
      pos_node->next_->prev_ = new_node;
    }
    pos_node->next_ = new_node;

    ++count_;
  }


  //
  // Removes
  //

 public:
  void Remove(Node* node) {
    fun_check(node);

    if (node->prev_) {
      node->prev_->next_ = node->next_;
    } else {
      head_ = node->next_;
    }

    if (node->next_) {
      node->next_->prev_ = node->prev_;
    } else {
      tail_ = node->prev_;
    }

    delete node;

    count_--;
  }

  bool RemoveOne(const ElementType& item_to_remove) {
    for (Node* node = head_; node; ) {
      if (node->value_ == item_to_remove) {
        Remove(node);
        return true;
      } else {
        node = node->next_;
      }
    }
    return false;
  }

  int32 RemoveAll(const ElementType& item_to_remove) {
    int32 removed_count = 0;
    for (Node* node = head_; node; ) {
      if (node->value_ == item_to_remove) {
        Node* next_ = node->next_;
        Remove(node);
        node = next_;

        ++removed_count;
      } else {
        node = node->next_;
      }
    }
    return removed_count;
  }

  int32 Remove(const ElementType& item_to_remove) {
    return RemoveAll(item_to_remove);
  }

  template <typename Predicate>
  bool RemoveOneIf(const Predicate& pred) {
    int32 removed_count = 0;
    for (Node* node = head_; node; ) {
      if (pred(node->value_)) {
        Remove(node);
        return true;
      } else {
        node = node->next_;
      }
    }
    return false;
  }

  template <typename Predicate>
  int32 RemoveAllIf(const Predicate& pred) {
    int32 removed_count = 0;
    for (Node* node = head_; node; ) {
      if (pred(node->value_)) {
        Node* next = node->next_;
        Remove(node);
        node = next;

        ++removed_count;
      } else {
        node = node->next_;
      }
    }
    return removed_count;
  }

  template <typename Predicate>
  int32 RemoveIf(const Predicate& pred) {
    return RemoveAllIf(pred);
  }

  void Remove(Iterator& iter) {
    fun_check(iter.IsValid());

    fun_check(iter.element_has_been_removed_ == false); // check duplicated removes
    iter.element_has_been_removed_ = true;

    Node* node = iter.current_node_;
    iter.current_node_ = iter.reversed_ ? node->prev_ : node->next_;

    Remove(node);
  }

  ElementType CutFront() {
    fun_check(head_);
    ElementType ret = MoveTemp(head_->value_);
    Remove(head_);
    return ret;
  }

  ElementType CutBack() {
    fun_check(tail_);
    ElementType ret = MoveTemp(tail_->value_);
    Remove(tail_);
    return ret;
  }

  void RemoveFront() {
    fun_check_ptr(head_);
    Remove(head_);
  }

  void RemoveBack() {
    fun_check_ptr(tail_);
    Remove(tail_);
  }

  void Clear() {
    for (Node* node = head_; node; ) {
      Node* next = node->next_;
      delete node;
      node = next;
    }
    head_ = tail_ = nullptr;
    count_ = 0;
  }


  //
  // Moves
  //

 public:
  void MoveBefore(const Iterator& position, const Iterator& item) {
    fun_check(item.IsValid());
    Node* item_node = item.current_node_;

    // unlink item.
    if (item_node->prev_) {
      item_node->prev_->next_ = item_node->next_;
    } else {
      head_ = item_node->next_;
    }
    if (item_node->next_) {
      item_node->next_->prev_ = item_node->prev_;
    } else {
      tail_ = item_node->prev_;
    }

    fun_check(position.IsValid());
    Node* pos_node = position.current_node_;
    Node* pos_next = pos_node;
    Node* pos_prev = pos_node->prev_;

    item_node->next_ = pos_next;
    item_node->prev_ = pos_prev;
    if (pos_prev == nullptr) { // this is the first element
      head_ = item_node;
    } else {
      pos_node->prev_->next_ = item_node;
    }
    pos_node->prev_ = item_node;
  }

  void MoveToFront(const Iterator& item) {
    fun_check(item.IsValid());
    Node* item_node = item.current_node_;

    if (item_node->prev_ == nullptr) { // already at first.
      return;
    }

    Node* pos_node = head_;
    Node* pos_next = pos_node;

    // unlink item.
    item_node->prev_->next_ = item_node->next_;
    if (item_node->prev_) {
      item_node->next_->prev_ = item_node->prev_;
    } else {
      tail_ = item_node->prev_;
    }

    item_node->next_ = pos_next;
    item_node->prev_ = nullptr;
    head_ = item_node;
    pos_node->prev_ = item_node;
  }

  void MoveAfter(const Iterator& position, const Iterator& item) {
    fun_check(item.IsValid());
    Node* item_node = item.current_node_;

    // unlink item.
    if (item_node->prev_) {
      item_node->prev_->next_ = item_node->next_;
    } else {
      head_ = item_node->next_;
    }
    if (item_node->next_) {
      item_node->next_->prev_ = item_node->prev_;
    } else {
      tail_ = item_node->prev_;
    }

    fun_check(position.IsValid());
    Node* pos_node = position.current_node_;
    Node* pos_next = pos_node->next_;
    Node* pos_prev = pos_node;

    item_node->next_ = pos_next;
    item_node->prev_ = pos_prev;
    if (pos_next == nullptr) { // this is the last element
      tail_ = item_node;
    } else {
      pos_node->next_->prev_ = item_node;
    }
    pos_node->next_ = item_node;
  }

  void MoveToBack(const Iterator& item) {
    fun_check(item.IsValid());
    Node* item_node = item.current_node_;

    if (item_node->next_ == nullptr) { // already at back.
      return;
    }

    Node* pos_node = tail_;
    Node* pos_prev = pos_node;

    // unlink item.
    if (item_node->prev_) {
      item_node->prev_->next_ = item_node->next_;
    } else {
      head_ = item_node->next_;
    }
    item_node->next_->prev_ = item_node->prev_;

    item_node->next_ = nullptr;
    item_node->prev_ = pos_prev;
    tail_ = item_node;
    pos_node->next_ = item_node;
  }


  //
  // Stack or Queue
  //

 public:
  void Enqueue(const ElementType& item) {
    Append(item);
  }

  void Enqueue(ElementType&& item) {
    Append(MoveTemp(item));
  }

  void Enqueue(const ElementType& item, int32 count) {
    Append(item, count);
  }

  void Enqueue(const List& list) {
    Append(list);
  }

  void Enqueue(std::initializer_list<ElementType> init_list) {
    Append(init_list);
  }

  bool Dequeue(ElementType& out_item) {
    return PopFront(out_item);
  }

  bool Dequeue(ElementType&& out_item) {
    return PopFront(out_item);
  }

  ElementType Dequeue() {
    return CutFront();
  }

  void PushFront(const ElementType& item) {
    Prepend(item);
  }

  void PushFront(ElementType&& item) {
    Prepend(MoveTemp(item));
  }

  ElementType& PushFrontDefault() {
    return PrependDefault();
  }

  void PushFront(const ElementType& item, int32 count) {
    Prepend(item, count);
  }

  void PushFront(const List& list) {
    Prepend(list);
  }

  void PushFront(std::initializer_list<ElementType> init_list) {
    Prepend(init_list);
  }

  void PushBack(const ElementType& item) {
    Append(item);
  }

  void PushBack(ElementType&& item) {
    Append(MoveTemp(item));
  }

  ElementType& PushBackDefault() {
    return AppendDefault();
  }

  void PushBack(const ElementType& item, int32 count) {
    Append(item, count);
  }

  void PushBack(const List& list) {
    Append(list);
  }

  void PushBack(std::initializer_list<ElementType> init_list) {
    Append(init_list);
  }

  bool PopFront() {
    if (head_) {
      Remove(head_);
      return true;
    }

    return false;
  }

  bool PopFront(ElementType& item) {
    if (head_) {
      item = head_->value_;
      Remove(head_);
      return true;
    }

    return false;
  }

  //bool PopFront(ElementType&& item) {
  //  if (head_) {
  //    item = MoveTemp(head_->value_);
  //    Remove(head_);
  //    return true;
  //  }
  //
  //  return false;
  //}

  bool PopBack() {
    if (tail_) {
      Remove(tail_);
      return true;
    }

    return false;
  }

  bool PopBack(ElementType& item) {
    if (tail_) {
      item = tail_->value_;
      Remove(tail_);
      return true;
    }

    return false;
  }

  //bool PopBack(ElementType&& item) {
  //  if (tail_) {
  //    item = MoveTemp(tail_->value_);
  //    Remove(tail_);
  //    return true;
  //  }
  //
  //  return false;
  //}


  //
  // Finds
  //

 public:
  Iterator Find(const ElementType& item) {
    return Iterator(FindNode(item));
  }

  ConstIterator Find(const ElementType& item) const {
    return ConstIterator(FindNode(item));
  }

  template <typename Predicate>
  Iterator FindIf(const Predicate& pred) {
    return Iterator(FindNodeIf(pred));
  }

  template <typename Predicate>
  ConstIterator FindIf(const Predicate& pred) const {
    return ConstIterator(FindNodeIf(pred));
  }

 private:
  Node* FindNode(const ElementType& item) {
    for (Node* node = head_; node; node = node->next_) {
      if (node->value_ == item) {
        return node;
      }
    }

    return nullptr;
  }

  const Node* FindNode(const ElementType& item) const {
    for (const Node* node = head_; node; node = node->next_) {
      if (node->value_ == item) {
        return node;
      }
    }

    return nullptr;
  }

  template <typename Predicate>
  Node* FindNodeIf(const Predicate& pred) {
    for (Node* node = head_; node; node = node->next_) {
      if (pred(node->value_)) {
        return node;
      }
    }

    return nullptr;
  }

  template <typename Predicate>
  const Node* FindNodeIf(const Predicate& pred) const {
    for (const Node* node = head_; node; node = node->next_) {
      if (pred(node->value_)) {
        return node;
      }
    }

    return nullptr;
  }


  //
  // Contains
  //

 public:
  bool Contains(const ElementType& item) const {
    return !!FindNode(item);
  }

  template <typename Predicate>
  bool ContainsIf(const Predicate& pred) const {
    return !!FindNodeIf(pred);
  }

  int32 Count(const ElementType& item) const {
    int32 matched_count = 0;
    for (const Node* node = head_; node; node = node->next_) {
      if (node->value_ == item) {
        ++matched_count;
      }
    }

    return matched_count;
  }

  template <typename Predicate>
  int32 CountIf(const Predicate& pred) const {
    int32 matched_count = 0;
    for (const Node* node = head_; node; node = node->next_) {
      if (pred(node->value_)) {
        ++matched_count;
      }
    }

    return matched_count;
  }

  int32 IndexOf(const ElementType& item) const {
    int32 index = 0;
    for (const Node* node = head_; node; node = node->next_) {
      if (node->value_ == item) {
        return index;
      }
      ++index;
    }

    return INVALID_INDEX;
  }

  template <typename Predicate>
  int32 IndexOfIf(const Predicate& pred) const {
    int32 index = 0;
    for (const Node* node = head_; node; node = node->next_) {
      if (pred(node->value_)) {
        return index;
      }
      ++index;
    }

    return INVALID_INDEX;
  }


  //
  // Accessors
  //

  bool IsEmpty() const {
    return count_ == 0;
  }

  int32 Count() const {
    return count_;
  }

  const ElementType& Front() const {
    fun_check_ptr(head_);
    return head_->value_;
  }

  ElementType& Front() {
    fun_check_ptr(head_);
    return head_->value_;
  }

  const ElementType& Back() const {
    fun_check_ptr(tail_);
    return tail_->value_;
  }

  ElementType& Back() {
    fun_check_ptr(tail_);
    return tail_->value_;
  }


  //
  // Iterations
  //

  Iterator CreateIterator() {
    return Iterator(head_);
  }

  Iterator CreateIterator(const ElementType& item) {
    return Iterator(FindNode(item));
  }

  ConstIterator CreateConstIterator() const {
    return ConstIterator(head_);
  }

  ConstIterator CreateConstIterator(const ElementType& item) const {
    return ConstIterator(FindNode(item));
  }

  ReversedIterator CreateReversedIterator() {
    return ReversedIterator(tail_);
  }

  ReversedIterator CreateReversedIterator(const ElementType& item) {
    return ReversedIterator(FindNode(item));
  }

  ReversedConstIterator CreateReversedConstIterator() const {
    return ReversedConstIterator(tail_);
  }

  ReversedConstIterator CreateReversedConstIterator(const ElementType& item) const {
    return ReversedConstIterator(FindNode(item));
  }


  //
  // Comparisions
  //

 public:
  bool operator == (const List& rhs) const {
    if (Count() != rhs.Count()) {
      return false;
    }

    const Node* this_node = head_;
    const Node* other_node = rhs.head_;
    for (; this_node; this_node = this_node->next_, other_node = other_node->next_) {
      if (this_node->value_ != other_node->value_) {
        return false;
      }
    }

    return true;
  }

  bool operator != (const List& rhs) const {
    return !(*this == rhs);
  }

  //less, greater ??

  // Serializer
  friend Archive& operator & (Archive& ar, List& list) {
    if (ar.IsLoading()) {
      list.Clear();
      int32 count;
      ar & count;
      for (int32 i = 0; i < count; ++i) {
        ElementType tmp;
        ar & tmp;
        list.Append(MoveTemp(tmp));
      }
    } else {
      int32 count = list.Count();
      ar & count;
      for (Node* node = list.head_; node; node = node->next_) {
        ar & node->value_;
      }
    }
    return *this;
  }

  // Hash

  friend uint32 HashOf(const List& list) {
    uint32 hash = 0;
    for (const Node* node = list.head_; node; node = node->next_) {
      hash ^= HashOf(node->value_);
    }

    return hash;
  }

 public:
  void CheckConsistency() const {
    //Count가 0일 경우, head_, Tail이 모두 nullptr이어야함.
    //TODO
  }


  //
  // STL Compatibilities
  //

 public:
  Iterator begin() { return Iterator(head_); }
  Iterator end() { return Iterator(nullptr); }
  ConstIterator begin() const { return ConstIterator(head_); }
  ConstIterator end() const { return ConstIterator(nullptr); }

  ReversedIterator rbegin() { return ReversedIterator(tail_); }
  ReversedIterator rend() { return ReversedIterator(nullptr); }
  ReversedConstIterator rbegin() const { return ReversedConstIterator(tail_); }
  ReversedConstIterator rend() const { return ReversedConstIterator(nullptr); }

 private:
  Node* head_;
  Node* tail_;
  int32 count_;
};


/**
 * Simple single-linked list template.
 */
template <typename _ElementType>
class SingleLinkedList {
 public:
  using ElementType = _ElementType;

  ElementType element;
  SingleLinkedList<ElementType>* next;

  SingleLinkedList(const ElementType& element, SingleLinkedList<ElementType>* next = nullptr)
    : element(element)
    , next(next) {}
};


template <typename _ElementType>
class ListNode {
 public:
  using ElementType = _ElementType;

  class ListOwner {
   public:
    using ElementType = _ElementType;
    using NodeType = ListNode<_ElementType>;

    ElementType* list_head_;
    ElementType* list_tail_;
    int32 list_count_;

   public:
    FUN_ALWAYS_INLINE ListOwner()
      : list_head_(nullptr), list_tail_(nullptr), list_count_(0) {}

    FUN_ALWAYS_INLINE ~ListOwner() {
      Clear();
    }

    FUN_ALWAYS_INLINE void Clear() {
      while (!IsEmpty()) {
        Unlink(list_head_);
      }
    }

    FUN_ALWAYS_INLINE bool IsEmpty() const {
      return list_head_ == nullptr;
    }

    FUN_ALWAYS_INLINE void Unlink(ElementType* node) {
      fun_check_ptr(node);
      fun_check(node->list_owner_ == this);

      ElementType* next_node = node->next_;
      ElementType* prev_node = node->prev_;
      if (next_node) {
        next_node->prev_ = prev_node;
      }

      if (prev_node) {
        prev_node->next_ = next_node;
      }

      if (next_node == nullptr) {
        fun_check(list_tail_ == node);
        list_tail_ = prev_node;
      }

      if (prev_node == nullptr) {
        fun_check(list_head_ == node);
        list_head_ = next_node;
      }

      node->prev_ = nullptr;
      node->next_ = nullptr;
      node->list_owner_ = nullptr;

      fun_check(list_count_ > 0);
      --list_count_;
    }

    FUN_ALWAYS_INLINE void Prepend(ElementType* node) {
      if (node->list_owner_ == nullptr) { // prevent circular.
        fun_check(0);
        Unlink(node); // prevent circular.
      }

      if (list_head_ == nullptr) {
        fun_check_dbg(list_tail_ == nullptr);
        list_head_ = list_tail_ = node;
      } else {
        list_head_->prev_ = node;
        node->next_ = list_head_;
        list_head_ = node;
      }

      node->list_owner_ = this;
      ++list_count_;
    }

    FUN_ALWAYS_INLINE void Append(ElementType* node) {
      if (node->list_owner_) { // prevent circular.
        fun_check(0);
        Unlink(node); // prevent circular.
      }

      if (list_tail_ == nullptr) {
        fun_check_dbg(list_head_ == nullptr);
        list_head_ = list_tail_ = node;
      } else {
        list_tail_->next_ = node;
        node->prev_ = list_tail_;
        list_tail_ = node;
      }

      node->list_owner_ = this;
      ++list_count_;
    }

    FUN_ALWAYS_INLINE ElementType* Front() const {
      return (ElementType*)list_head_;
    }

    FUN_ALWAYS_INLINE ElementType* Back() const {
      return (ElementType*)list_tail_;
    }

    FUN_ALWAYS_INLINE int32 Count() const {
      return list_count_;
    }
  };

 private:
  friend class ListOwner;

  ElementType* prev_;
  ElementType* next_;
  ListOwner* list_owner_;

 public:
  FUN_ALWAYS_INLINE ListNode()
    : prev_(nullptr)
    , next_(nullptr)
    , list_owner_(nullptr) {}

  //FUN_ALWAYS_INLINE virtual ~ListNode() {
  //  UnlinkSelf();
  //}
  FUN_ALWAYS_INLINE ~ListNode() {
    UnlinkSelf();
  }

  FUN_ALWAYS_INLINE ListOwner* GetListOwner() const {
    return list_owner_;
  }

  FUN_ALWAYS_INLINE ElementType* GetNextNode() const {
    return (ElementType*)next_;
  }

  FUN_ALWAYS_INLINE ElementType* GetPrevNode() const {
    return (ElementType*)prev_;
  }

  //TODO 이 함수는 제거하는게 좋을듯...??
  FUN_ALWAYS_INLINE void UnlinkSelf() {
    if (list_owner_) {
      list_owner_->Unlink((ElementType*)this);
    }
  }
};

} // namespace fun
