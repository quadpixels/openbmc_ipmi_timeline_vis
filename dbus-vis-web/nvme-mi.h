#ifndef NVME_MI_H
#define NVME_MI_H

struct mctp_header {
		uint8_t ver_resv;
		uint8_t dest_eid;
		uint8_t src_eid;
		uint8_t flags;
};

struct nvme_mi_header {
	    uint8_t byte0;
#define GET_MCTP_MSG_TYPE(v)    ((v) & 0x3f)
#define GET_MCTP_IC(v)          (((v) >> 7) & 0x1)
		uint8_t byte1;
#define GET_MI_CMD_SLOT(v)  (((v) >> 0) & 0x1)
#define GET_MI_TYPE(v)      (((v) >> 3) & 0xf)
#define GET_MI_RBIT(v)      (((v) >> 7) & 0x1)
		uint8_t byte2;
#define GET_MI_MEB(v)       (((v) >> 0) & 0x1)
#define GET_MI_CIAP(v)      (((v) >> 1) & 0x1)
		uint8_t byte3;
};

struct MCTPRequest {
	double t0 = 0, t1 = 0;
	int from_eid, to_eid;
    int frag_count = 0;

	std::string desc;

	std::string ToString() const {
		char buf[100];
		if (!IsIncomplete()) {
			snprintf(buf, 100, "%.3f, %.3f, \"%s\", TRUE", t0, t1, desc.c_str());
		} else {
			snprintf(buf, 100, "%.3f, %.3f, \"%s\", FALSE", t0, t0, desc.c_str());
		}
		return std::string(buf);
	}

	bool IsIncomplete() const {
		return t1 == 0;
	}
};

#endif