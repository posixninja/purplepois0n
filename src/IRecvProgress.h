/*
 * IRecvProgress.h
 *
 * Optional IRECV_PROGRESS subscription for long DFU uploads (libirecovery).
 */

#ifndef IRECV_PROGRESS_H_
#define IRECV_PROGRESS_H_

#include <libirecovery.h>
#include <functional>
#include <memory>

namespace PP {

/** Reports upload progress in [0.0, 1.0] when libirecovery emits IRECV_PROGRESS. */
using IRecvProgressCallback = std::function<void(double progress)>;

/** RAII irecv_event_subscribe(IRECV_PROGRESS) wrapper. */
class IRecvProgressSubscription {
public:
    IRecvProgressSubscription(irecv_client_t client, IRecvProgressCallback callback);
    ~IRecvProgressSubscription();

    IRecvProgressSubscription(const IRecvProgressSubscription&) = delete;
    IRecvProgressSubscription& operator=(const IRecvProgressSubscription&) = delete;

private:
    static int eventCallback(irecv_client_t client, const irecv_event_t* event);

    irecv_client_t m_client;
    std::shared_ptr<IRecvProgressCallback> m_callback;
};

} /* namespace PP */

#endif /* IRECV_PROGRESS_H_ */
