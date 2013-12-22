#include "BWebSocket.h"
#include "SHA1.h"
#include "Base64.h"
#include "TaskMan.h"

#define rotate(value) (((value) << 8) | ((value) >> 24))

void Balau::WebSocketWorker::Do() {
    uint8_t c;

    try {
        while (!m_socket->isClosed()) {
            switch (m_state) {
            case READ_H:
                c = m_socket->readU8().get();
                m_fin = c & 0x80;
                if ((c >> 4) & 7) goto error;
                c &= 15;
                if (!m_firstFragment && c) goto error;
                if (m_firstFragment)
                    m_opcode = c;
                m_state = READ_PLB;
            case READ_PLB:
                c = m_socket->readU8().get();
                m_hasMask = c & 0x80;
                if (m_enforceServer && !m_hasMask)
                    goto error;
                if (m_enforceClient && m_hasMask)
                    goto error;
                m_payloadLen = c & 0x7f;
                m_state = READ_PLL;
                if (m_payloadLen == 126) {
                    m_payloadLen = 0;
                    m_remainingBytes = 2;
                }
                else if (m_payloadLen == 127) {
                    m_payloadLen = 0;
                    m_remainingBytes = 8;
                }
                else {
                    m_remainingBytes = 0;
                }
            case READ_PLL:
                while (m_remainingBytes) {
                    c = m_socket->readU8().get();
                    m_payloadLen <<= 8;
                    m_payloadLen += c;
                    m_remainingBytes--;
                }
                m_state = READ_MK;
                if (m_firstFragment)
                    m_totalLen = m_payloadLen;
                else
                    m_totalLen += m_payloadLen;
                if (m_hasMask) m_remainingBytes = 4;
            case READ_MK:
                while (m_remainingBytes) {
                    c = m_socket->readU8().get();
                    m_mask <<= 8;
                    m_mask += c;
                    m_remainingBytes--;
                }
                m_state = READ_PL;
                m_remainingBytes = m_payloadLen;
                if (m_totalLen >= MAX_WEBSOCKET_LIMIT)
                    goto error;
                m_payload = (uint8_t *)realloc(m_payload, m_totalLen);
            case READ_PL:
                while (m_remainingBytes) {
                    int r = m_socket->read(m_payload + m_totalLen - m_remainingBytes, m_remainingBytes);
                    if (r < 0)
                        goto error;
                    m_remainingBytes -= r;
                }

                m_firstFragment = m_fin;

                if (m_fin) {
                    if (m_hasMask) {
                        for (int i = 0; i < m_totalLen; i++) {
                            m_payload[i] ^= m_mask >> 24;
                            m_mask = rotate(m_mask);
                        }
                    }
                    processMessage();
                }
            }
        }

    error:
        m_socket->close();
    }
    catch (Balau::EAgain & e) {
        taskSwitch();
    }
}

void Balau::WebSocketWorker::processMessage() {

}

void Balau::WebSocketServerBase::sendError(IO<Handle> out, const char * serverName) {
    const char * status = Http::getStatusMsg(400);
    String errorMsg;
    errorMsg.set(
"HTTP/1.0 400 %s\r\n"
"Content-Type: text/plain; charset=UTF-8\r\n"
"Connection: close\r\n"
"Server: %s\r\n"
"\r\n"
"400 - %s",
        status, serverName, status);
    out->writeString(errorMsg);
}

static const Balau::String magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

bool Balau::WebSocketServerBase::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    WebSocketWorker * worker = NULL;

    if (!req.upgrade)
        goto error;

    if (req.headers["Upgrade"] != "websocket")
        goto error;

    if (req.headers["Sec-WebSocket-Key"] == "")
        goto error;

    worker = spawnWorker(out, req.uri);
    if (!worker->parse(req))
        goto error;

    TaskMan::registerTask(worker);
    {
        HttpServer::Response response(server, req, out);

        String & key = req.headers["Sec-WebSocket-Key"];
        uint8_t * toHash = (uint8_t *)alloca(key.strlen() + magic.strlen());
        memcpy(toHash, key.to_charp(), key.strlen());
        memcpy(toHash + key.strlen(), magic.to_charp(), magic.strlen());

        SHA1 h;
        uint8_t digest[20];
        h.update(toHash, key.strlen() + magic.strlen());
        h.final(digest);

        String accept = Base64::encode(digest, 20);

        response.SetResponseCode(101);
        response.AddHeader("Upgrade: websocket");
        response.AddHeader("Connection: Upgrade");
        response.AddHeader("Sec-WebSocket-Accept", accept);
        response.AddHeader("Sec-WebSocket-Version: 13");
        response.SetContentType("");
        response.Flush();
    }

    return false;

error:
    if (worker)
        TaskMan::registerTask(worker);
    sendError(out, server->getServerName().to_charp());
    return false;
}
