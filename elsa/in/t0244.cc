// t0244.cc   


template <class CharT>
class nsSharedBufferHandle
{
    void AcquireReference() const
    {
      this;
    }
};



void f()
{
  nsSharedBufferHandle<unsigned short> *mHandle;
  mHandle->AcquireReference();
}

