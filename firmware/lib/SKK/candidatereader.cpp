#include "candidatereader.h"


#include <debug.h>


using namespace SKK;

void CandidateReader::init(SkkDict* parent, uint8_t candidatescnt, uint16_t candidateslen, uint32_t startaddr) {
    this->parentDict = parent;
    this->candidates_count = candidatescnt;
    this->candidateslen = candidateslen;
    this->startaddr = startaddr;
    this->parentDict->file->seek(startaddr);
    this->current_candidate_head = startaddr;
    this->current_candidate_len = (uint8_t)this->parentDict->file->read_uint8();
    this->current_remains = this->current_candidate_len;
    this->current_candidate_count = 1;
    // this->move_next();
    // DEBUG("count=%d, len=%d, addr=%ld", this->candidates_count, this->candidateslen, this->startaddr);
    // DEBUG("current candidate: len=%d, remains=%d", (uint8_t)this->current_candidate_len, (uint8_t)this->current_remains);
}

uint8_t CandidateReader::get_candidates_count(void) {
    return this->candidates_count;
}


uint16_t CandidateReader::get_current_candidate_length(void) {
    return this->current_candidate_len;
}

int CandidateReader::read(void) {
    if (this->is_reached_end()) {
        // DEBUG("Reached to end.");
        return -1;
    } else if (this->current_remains == 0) {
        // DEBUG("No byte remains in this candidate.");
        return -1;
    } else {
        this->current_remains -= 1;
        return this->parentDict->file->read_uint8();
    }
}

bool CandidateReader::move_next(void) {
    if (this->is_reached_end()) {
        DEBUG("return false due to is_reached_end() == true.");
        return false;
    }

    // Seek to next head.
    // uint32_t next_candidate_head_addr = this->current_candidate_head + 1 + this->current_candidate_len;
    // this->parentDict->file->seek(next_candidate_head_addr);
    // this->parentDict->file->seek_delta(this->current_candidate_len + 1);
    while (this->current_remains > 0) {
        (void)this->read();
    }

    // Set fields.
    this->current_candidate_head = this->parentDict->file->position();
    this->current_candidate_len = this->parentDict->file->read_uint8();
    this->current_remains = this->current_candidate_len;
    this->current_candidate_count += 1;
    DEBUG("moved. count=%d, len=%d, addr=%ld", this->candidates_count, this->candidateslen, this->startaddr);
    return true;
}

bool CandidateReader::move_head(void) {
    this->parentDict->file->seek(this->startaddr);
    this->current_candidate_head = this->startaddr;
    this->current_candidate_len = this->parentDict->file->read_uint8();
    this->current_remains = this->current_candidate_len;
    this->current_candidate_count = 1;
    DEBUG("Move to head.");
    return true;
}

bool CandidateReader::is_reached_end(void) {
    return this->current_candidate_count > this->candidates_count;
}
