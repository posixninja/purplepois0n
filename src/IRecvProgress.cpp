/*
 * IRecvProgress.cpp
 */

#include "IRecvProgress.h"

#include <map>
#include <mutex>

namespace PP {

namespace {

std::mutex g_progressMutex;
std::map<irecv_client_t, std::shared_ptr<IRecvProgressCallback>> g_progressHandlers;

} /* anonymous */

IRecvProgressSubscription::IRecvProgressSubscription(irecv_client_t client,
                                                     IRecvProgressCallback callback)
    : m_client(client), m_callback(std::make_shared<IRecvProgressCallback>(std::move(callback))) {
    if (m_client == nullptr || !m_callback || !*m_callback) {
        m_callback.reset();
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_progressMutex);
        g_progressHandlers[m_client] = m_callback;
    }
    irecv_event_subscribe(m_client, IRECV_PROGRESS, eventCallback, nullptr);
}

IRecvProgressSubscription::~IRecvProgressSubscription() {
    if (m_client == nullptr) {
        return;
    }
    irecv_event_unsubscribe(m_client, IRECV_PROGRESS);
    std::lock_guard<std::mutex> lock(g_progressMutex);
    g_progressHandlers.erase(m_client);
}

int IRecvProgressSubscription::eventCallback(irecv_client_t client, const irecv_event_t* event) {
    if (event == nullptr || event->type != IRECV_PROGRESS) {
        return 0;
    }
    std::shared_ptr<IRecvProgressCallback> callback;
    {
        std::lock_guard<std::mutex> lock(g_progressMutex);
        const auto it = g_progressHandlers.find(client);
        if (it == g_progressHandlers.end()) {
            return 0;
        }
        callback = it->second;
    }
    if (callback && *callback) {
        (*callback)(event->progress);
    }
    return 0;
}

} /* namespace PP */
