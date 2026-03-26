



















#ifndef RAPIDJSON_INTERNAL_STACK_H_
#define RAPIDJSON_INTERNAL_STACK_H_

namespace rapidjson {
namespace internal {






template <typename Allocator>
class Stack {
public:
    
    
    Stack(Allocator* allocator, size_t stackCapacity) : allocator_(allocator), ownAllocator(0), stack_(0), stackTop_(0), stackEnd_(0), initialCapacity_(stackCapacity) {
        RAPIDJSON_ASSERT(stackCapacity > 0);
        if (!allocator_)
            ownAllocator = allocator_ = new Allocator();
    }

    ~Stack() {
        Allocator::Free(stack_);
        delete ownAllocator; 
    }

    void Clear() { stackTop_ = stack_; }

    void ShrinkToFit() { 
        if (Empty()) {
            
            Allocator::Free(stack_);
            stack_ = 0;
            stackTop_ = 0;
            stackEnd_ = 0;
        }
        else
            Resize(GetSize());
    }

    
    
    template<typename T>
    RAPIDJSON_FORCEINLINE T* Push(size_t count = 1) {
         
        if (stackTop_ + sizeof(T) * count >= stackEnd_)
            Expand<T>(count);

        T* ret = reinterpret_cast<T*>(stackTop_);
        stackTop_ += sizeof(T) * count;
        return ret;
    }

    template<typename T>
    T* Pop(size_t count) {
        RAPIDJSON_ASSERT(GetSize() >= count * sizeof(T));
        stackTop_ -= count * sizeof(T);
        return reinterpret_cast<T*>(stackTop_);
    }

    template<typename T>
    T* Top() { 
        RAPIDJSON_ASSERT(GetSize() >= sizeof(T));
        return reinterpret_cast<T*>(stackTop_ - sizeof(T));
    }

    template<typename T>
    T* Bottom() { return (T*)stack_; }

    Allocator& GetAllocator() { return *allocator_; }
    bool Empty() const { return stackTop_ == stack_; }
    size_t GetSize() const { return static_cast<size_t>(stackTop_ - stack_); }
    size_t GetCapacity() const { return static_cast<size_t>(stackEnd_ - stack_); }

private:
    template<typename T>
    void Expand(size_t count) {
        
        size_t newCapacity;
        if (stack_ == 0)
            newCapacity = initialCapacity_;
        else {
            newCapacity = GetCapacity();
            newCapacity += (newCapacity + 1) / 2;
        }
        size_t newSize = GetSize() + sizeof(T) * count;
        if (newCapacity < newSize)
            newCapacity = newSize;

        Resize(newCapacity);
    }

    void Resize(size_t newCapacity) {
        const size_t size = GetSize();  
        stack_ = (char*)allocator_->Realloc(stack_, GetCapacity(), newCapacity);
        stackTop_ = stack_ + size;
        stackEnd_ = stack_ + newCapacity;
    }

    
    Stack(const Stack&);
    Stack& operator=(const Stack&);

    Allocator* allocator_;
    Allocator* ownAllocator;
    char *stack_;
    char *stackTop_;
    char *stackEnd_;
    size_t initialCapacity_;
};

} 
} 

#endif 
