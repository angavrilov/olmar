// /home/ballAruns/tmpfiles/./aspell-0.33.7.1-21/email-MKxI.i:29703:12:
// Parse error (state 141) at class

struct A {};
template <class T> class C {};
template                        // the presence of this 'template' keyword causes the failure
  class C<A>;
