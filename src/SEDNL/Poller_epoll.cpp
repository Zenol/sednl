// SEDNL - Copyright (c) 2013 Jeremy S. Cochoy
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//     1. The origin of this software must not be misrepresented; you must not
//        claim that you wrote the original software. If you use this software
//        in a product, an acknowledgment in the product documentation would
//        be appreciated but is not required.
//
//     2. Altered source versions must be plainly marked as such, and must not
//        be misrepresented as being the original software.
//
//     3. This notice may not be removed or altered from any source
//        distribution.

// This is the linux epoll backend

// This flag is activated at compile time
#ifdef SEDNL_BACKEND_EPOLL

#include "SEDNL/Types.hpp"
#include "SEDNL/Poller.hpp"

#include <cstring>

namespace SedNL
{

Poller::Poller()
    :m_epoll(-1),
     m_nb_events(0), m_idx(0)
{
    bzero(m_events, sizeof(*m_events) * MAX_EVENTS);
    //Create epoll
    m_epoll = epoll_create(EPOLL_SIZE);
    if (m_epoll < 0)
        throw EventException(EventExceptionT::PollerCreateFailed);
}

Poller::~Poller()
{
    if (m_epoll != -1)
        close(m_epoll);
}

//Close fd befor throwing, to prevent resource licking
bool Poller::add_fd(FileDescriptor fd) noexcept
{
    struct epoll_event event;

    if (m_epoll < 0)
        return false;

    bzero(&event, sizeof(event));
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;

    const int err = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &event);
    if (err < 0)
        return false;
    return true;
}

void Poller::remove_fd(FileDescriptor /*fd*/) noexcept
{
    /*
      EPOLL automatically remove closed FDs.
    */
}

void Poller::wait_for_events(int timeout) noexcept
{
    m_nb_events = epoll_wait(m_epoll, m_events, MAX_EVENTS, timeout);
    m_idx = 0;
}

bool Poller::next_event(Event& e) noexcept
{
    if (m_idx >= m_nb_events)
        return false;

    e.fd = m_events[m_idx].data.fd;
    //An error occured or the connection was closed
    e.is_close = m_events[m_idx].events & EPOLLERR
        || m_events[m_idx].events & EPOLLHUP
        || m_events[m_idx].events & EPOLLRDHUP;
    //Ready to read
    e.is_read = m_events[m_idx].events & EPOLLIN;

    m_idx++;
    return true;
}

} // namespace SedNL

#endif /* SEDNL_BACKEND_EPOLL */
