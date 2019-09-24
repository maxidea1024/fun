#pragma once

#include "fun/base/base.h"
#include "fun/base/atomics.h"

namespace fun {

/**
 * Enumerates concurrent queue modes.
 */
enum class QueueMode : uint8 {
  /** Multiple-producers, single-consumer queue. */
  Mpsc,

  /** Single-producer, single-consumer queue. */
  Spsc
};


/**
 * Template for queues.
 *
 * This template implements an unbounded non-intrusive queue using a lock-free linked
 * list that stores copies of the queued items. The template can operate in two modes:
 * Multiple-producers single-consumer (MPSC) and Single-producer single-consumer (SPSC).
 *
 * The queue is thread-safe in both modes. The Dequeue() method ensures thread-safety by
 * writing it in a way that does not depend on possible instruction reordering on the CPU.
 * The Enqueue() method uses an atomic compare-and-swap in multiple-producers scenarios.
 *
 * \param _ItemType - The type of items stored in the queue.
 * \param Mode - The queue mode.
 */
template <typename _ItemType, QueueMode Mode = QueueMode::Mpsc>
class Queue
{
 public:
  using ItemType = _ItemType;

  /**
   * Default constructor.
   */
  Queue()
  {
    head_ = tail_ = new Node();
  }

  /**
   * Destructor.
   */
  ~Queue()
  {
    while (tail_) {
      Node* node = tail_;
      tail_ = tail_->next_node;
      delete node;
    }
  }

  // Disable copy and assignment.
  Queue(const Queue&) = delete;
  Queue& operator = (const Queue&) = delete;

  /**
   * Removes and returns the item from the tail of the queue.
   *
   * \param out_item - Will hold the returned value.
   *
   * \return true if a value was returned, false if the queue was empty.
   *
   * \see Enqueue, IsEmpty, Peek
   */
  bool Dequeue(ItemType& out_item)
  {
    Node* popped = tail_->next_node;

    if (popped == nullptr) {
      return false;
    }

    out_item = popped->item;

    Node* old_tail = tail_;
    tail_ = popped;
    tail_->item = ItemType();
    delete old_tail;

    return true;
  }

  /**
   * Empty the queue, discarding all items.
   */
  void Clear()
  {
    ItemType ignorant;
    while (Dequeue(ignorant));
  }

  /**
   * Adds an item to the head of the queue.
   *
   * \param item - The item to add.
   *
   * \return true if the item was added, false otherwise.
   *
   * \see Dequeue, IsEmpty, Peek
   */
  bool Enqueue(const ItemType& item)
  {
    Node* new_node = new Node(item);

    if (new_node == nullptr) {
      return false;
    }

    Node* old_head;

    if (Mode == QueueMode::Mpsc) {
      old_head = (Node*)Atomics::ExchangePtr((void**)&head_, new_node);
    }
    else {
      old_head = head_;
      head_ = new_node;
    }

    old_head->next_node = new_node;

    return true;
  }

  /**
   * Checks whether the queue is empty.
   *
   * \return true if the queue is empty, false otherwise.
   *
   * \see Dequeue, Enqueue, Peek
   */
  bool IsEmpty() const
  {
    return tail_->next_node == nullptr;
  }

  /**
   * Peeks at the queue's tail item without removing it.
   *
   * \param out_item - Will hold the peeked at item.
   *
   * \return true if an item was returned, false if the queue was empty.
   *
   * \see Dequeue, Enqueue, IsEmpty
   */
  bool Peek(ItemType& out_item)
  {
    if (tail_->next_node == nullptr) {
      return false;
    }

    out_item = tail_->next_node->item;

    return true;
  }

 private:
  /**
   * Structure for the internal linked list.
   */
  struct Node {
    /** Holds a pointer to the next node in the list. */
    Node* volatile next_node;

    /** Holds the node's item. */
    ItemType item;

    /**
     * Default constructor.
     */
    Node()
      : next_node(nullptr)
    {}

    /**
     * Creates and initializes a new node.
     */
    Node(const ItemType& item)
      : next_node(nullptr)
      , item(item)
    {}
  };

  /** Holds a pointer to the head of the list. */
  alignas(16) Node* volatile head_;

  /** Holds a pointer to the tail of the list. */
  Node* tail_;
};

} // namespace fun
