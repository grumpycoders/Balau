#include "BWebSocket.h"
#include "BHashes.h"
#include "Base64.h"
#include "TaskMan.h"

#define rotate(value) (((value) << 8) | ((value) >> 24))

Balau::WebSocketFrame::WebSocketFrame(const uint8_t * data, size_t len, uint8_t opcode, bool doMask) {
    m_len = len;
    m_headerSize = 2;
    if (m_len >= 126)   m_headerSize += 2;
    if (m_len >= 65536) m_headerSize += 6;
    if (doMask)         m_headerSize += 4;
    m_data = (uint8_t *)malloc(m_len + m_headerSize);
    uint8_t * maskPtr;

    m_data[0] = 0x80 | opcode;
    m_data[1] = doMask ? 0x80 : 0x00;
    if (m_len < 125) {
        m_data[1] |= m_len;
        maskPtr = m_data + 2;
    } else if (m_len < 65536) {
        m_data[1] |= 126;
        m_data[2] = m_len >> 8;
        m_data[3] = m_len & 0xff;
        maskPtr = m_data + 4;
    } else {
        m_data[1] |= 127;
        uint8_t * lenPtr = maskPtr = m_data + 10;
        size_t len = m_len;
        *(--lenPtr) = len & 0xff; len >>= 8;
        *(--lenPtr) = len & 0xff; len >>= 8;
        *(--lenPtr) = len & 0xff; len >>= 8;
        *(--lenPtr) = len & 0xff; len >>= 8;
        *(--lenPtr) = len & 0xff; len >>= 8;
        *(--lenPtr) = len & 0xff; len >>= 8;
        *(--lenPtr) = len & 0xff; len >>= 8;
        *(--lenPtr) = len & 0xff; len >>= 8;
    }

    if (doMask) {
        uint32_t mask = m_mask;
        for (int i = 0; i < 4; i++) {
            *(maskPtr++) = mask >> 24;
            mask <<= 8;
        }
    } else {
        m_mask = 0;
    }

    if (data) memcpy(maskPtr, data, m_len);
}

uint8_t & Balau::WebSocketFrame::operator[](size_t idx) {
    static uint8_t dummy = 0;
    if (idx >= m_len) return dummy;
    return m_data[idx + m_headerSize];
}

void Balau::WebSocketFrame::send(Balau::IO<Balau::Handle> socket) {
    size_t totalLen = m_headerSize + m_len;

    if (m_mask) {
        for (int i = m_headerSize; i < totalLen; i++) {
            m_data[i] ^= m_mask >> 24;
            m_mask = rotate(m_mask);
        }
        m_mask = 0;
    }

    while (m_bytesSent < totalLen) {
        ssize_t r = socket->write(m_data + m_bytesSent, totalLen - m_bytesSent);
        if (r < 0)
            m_bytesSent = totalLen;
        else
            m_bytesSent += r;
    }
}

Balau::WebSocketWorker::~WebSocketWorker() {
    free(m_payload);
    free(m_payloadCTRL);
    delete m_sending;
    while (!m_sendQueue.isEmpty())
        delete m_sendQueue.pop();
}

void Balau::WebSocketWorker::Do() {
    uint8_t c;

    uint8_t ** payloadP;
    uint64_t * payloadLenP;
    uint64_t * totalLenP;
    uint64_t * remainingBytesP;
    uint32_t * maskP;
    uint8_t * opcodeP;
    bool * hasMaskP;
    int fin;

    std::function<void()> switchPacketType = [&]() {
        if (m_inCTRL) {
            payloadP = &m_payloadCTRL;
            payloadLenP = &m_payloadLenCTRL;
            totalLenP = &m_totalLenCTRL;
            remainingBytesP = &m_remainingBytesCTRL;
            maskP = &m_maskCTRL;
            opcodeP = &m_opcodeCTRL;
            hasMaskP = &m_hasMaskCTRL;
        } else {
            payloadP = &m_payload;
            payloadLenP = &m_payloadLen;
            totalLenP = &m_totalLen;
            remainingBytesP = &m_remainingBytes;
            maskP = &m_mask;
            opcodeP = &m_opcode;
            hasMaskP = &m_hasMask;
        }
    };

    switchPacketType();

    waitFor(m_sendQueue.getEvent());
    m_sendQueue.getEvent()->resetMaybe();

    try {
        while (!m_socket->isClosed()) {
            for (;;) {
                if (m_sending)
                    m_sending->send(m_socket);

                if (!m_sendQueue.isEmpty())
                    m_sending = m_sendQueue.pop();
                else
                    break;
                if (m_socket->isClosed())
                    return;
            }

            delete m_sending;
            m_sending = NULL;

            switch (m_state) {
            case READ_H:
                m_socket->read(&c, 1);
                if (m_socket->isClosed())
                    return;
                fin = c & 0x80;
                if ((c >> 4) & 7)
                    goto error;
                c &= 15;
                if (!m_firstFragment && c)
                    goto error;
                if (m_firstFragment) {
                    bool wasInCtrl = m_inCTRL;
                    m_inCTRL = c & 8;
                    if (wasInCtrl != m_inCTRL)
                        switchPacketType();
                    *opcodeP = c;
                } else {
                    bool wasInCtrl = m_inCTRL;
                    m_inCTRL = false;
                    if (wasInCtrl != m_inCTRL)
                        switchPacketType();
                }
                if (!m_inCTRL)
                    m_fin = fin;
                else if (!fin)
                    goto error;
                m_state = READ_PLB;
            case READ_PLB:
                m_socket->read(&c, 1);
                if (m_socket->isClosed())
                    return;
                *hasMaskP = c & 0x80;
                if (m_enforceServer && !*hasMaskP)
                    goto error;
                if (m_enforceClient && *hasMaskP)
                    goto error;
                *payloadLenP = c & 0x7f;
                m_state = READ_PLL;
                if (*payloadLenP == 126) {
                    *payloadLenP = 0;
                    *remainingBytesP = 2;
                } else if (*payloadLenP == 127) {
                    *payloadLenP = 0;
                    *remainingBytesP = 8;
                } else {
                    *remainingBytesP = 0;
                }
            case READ_PLL:
                while (*remainingBytesP) {
                    m_socket->read(&c, 1);
                    if (m_socket->isClosed())
                        return;
                    *payloadLenP <<= 8;
                    *payloadLenP += c;
                    *remainingBytesP -= 1;
                }
                m_state = READ_MK;
                if (m_firstFragment || m_inCTRL)
                    *totalLenP = *payloadLenP;
                else
                    *totalLenP += *payloadLenP;
                if (*hasMaskP) *remainingBytesP = 4;
            case READ_MK:
                while (*remainingBytesP) {
                    m_socket->read(&c, 1);
                    if (m_socket->isClosed())
                        return;
                    *maskP <<= 8;
                    *maskP += c;
                    *remainingBytesP -= 1;
                }
                m_state = READ_PL;
                *remainingBytesP = *payloadLenP;
                if (*totalLenP >= MAX_WEBSOCKET_LIMIT)
                    goto error;
                *payloadP = (uint8_t *)realloc(*payloadP, *totalLenP + (*opcodeP == OPCODE_TEXT ? 1 : 0));
            case READ_PL:
                while (*remainingBytesP) {
                    int r = m_socket->read(*payloadP + *totalLenP - *remainingBytesP, *remainingBytesP);
                    if (m_socket->isClosed())
                        return;
                    if (r < 0)
                        goto error;
                    *remainingBytesP -= r;
                }

                if (!m_inCTRL)
                    m_firstFragment = m_fin;

                if (m_fin || m_inCTRL) {
                    uint8_t * payload = *payloadP;
                    uint64_t totalLen = *totalLenP;
                    if (*hasMaskP) {
                        uint32_t mask = *maskP;
                        for (int i = 0; i < totalLen; i++) {
                            payload[i] ^= mask >> 24;
                            mask = rotate(mask);
                        }
                    }
                    if (*opcodeP == OPCODE_TEXT)
                        payload[totalLen] = 0;
                    processMessage();
                }

                m_state = READ_H;
            }
        }

    error:
        disconnect();
    }
    catch (Balau::EAgain &) {
        taskSwitch();
    }
}

void Balau::WebSocketWorker::processMessage() {
    switch (m_opcode) {
    case OPCODE_PING:
        processPing();
        break;
    case OPCODE_PONG:
        processPong();
        break;
    case OPCODE_TEXT:
    case OPCODE_BIN:
        receiveMessage(m_payload, m_payloadLen, m_opcode == OPCODE_BIN);
        break;
    default:
        disconnect();
    }
}

void Balau::WebSocketWorker::processPing() {
    sendFrame(new WebSocketFrame(m_payloadCTRL, m_payloadLenCTRL, OPCODE_PING, m_enforceClient));
}

void Balau::WebSocketWorker::processPong() {

}

void Balau::WebSocketWorker::enforceClient() throw (GeneralException) {
    if (m_enforceClient || m_enforceServer)
        throw GeneralException("Can't set client or server mode more than once");
    m_enforceClient = true;
}

void Balau::WebSocketWorker::enforceServer() throw (GeneralException) {
    if (m_enforceClient || m_enforceServer)
        throw GeneralException("Can't set client or server mode more than once");
    m_enforceServer = true;
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

    if (!req.upgrade) goto error;

    if (req.headers["Upgrade"].lower() != "websocket") goto error;

    if (req.headers["Sec-WebSocket-Key"] == "") goto error;

    worker = spawnWorker(out, req.uri);
    if (!worker->parse(req)) goto error;

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
