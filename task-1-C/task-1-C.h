class ticket_spinlock {
private:
    using ticket = std::size_t;
    
public:
    ticket_spinlock()
        : owner_ticket(0)
        , next_free_ticket(0)
    {}

    void lock() {
        ticket this_thread_ticket = next_free_ticket.fetch_add(1);
        while (owner_ticket.load() != this_thread_ticket) {
            // wait
        }
    }

    void unlock() {
        owner_ticket.store(owner_ticket.load() + 1);
    }

    bool try_lock() {
        if (owner_ticket.fetch_sub(1) == next_free_ticket.load()) {
            return true;
        }
        else {
            owner_ticket.fetch_add(1);
            return false;
        }
    }

private:
    std::atomic<ticket_t> owner_ticket;
    std::atomic<ticket_t> next_free_ticket;
};