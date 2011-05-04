
#ifndef __WAITMSG_H
#define __WAITMSG_H

namespace sos
{

struct Wait_msg_box
{
                                Wait_msg_box            ( Bool*, int4 timeout = -1,
                                                          const char* title=0 );
                               ~Wait_msg_box            ();
  private:
    struct Wait_msg_box_impl*  _impl_ptr;
};

} //namespace sos

#endif
