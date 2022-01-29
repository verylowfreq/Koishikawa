#include <FileAccessWrapper.h>
#include <ArduinoSDFileAccessor.h>

#include <Arduino.h>
#include <SD.h>

#include <panic.h>
#include <debug.h>
#include <commondef.h>


bool ArduinoSDFileAccessor::open(const char* path, FileMode mode) {
    if (mode != FileMode::READ) {
        PANIC("FileAccessor supports only FileMode::READ");
    }

    File f = SD.open(path, FILE_READ);
    if (!f) {
        DEBUG("Requested file cannot open. \"%s\"", path);
        return false;

    } else {
        this->file = f;
        this->mode = mode;
        this->_is_opened = true;
        return true;
    }
}

bool ArduinoSDFileAccessor::is_opened(void) {
    return this->_is_opened;
}

void ArduinoSDFileAccessor::close(void) {
    if (this->is_opened()) {
        this->file.close();
        this->_is_opened = false;
    }
}

int ArduinoSDFileAccessor::read(void) {
    if (!this->is_opened()) {
        return -1;
    } else {
        return this->file.read();
    }
}

uint32_t ArduinoSDFileAccessor::position(void) {
    if (!this->is_opened()) {
        return INVALID_UINT32;
    } else {
        return this->file.position();
    }
}

uint32_t ArduinoSDFileAccessor::seek(uint32_t pos) {
    if (!this->_is_opened) {
        return INVALID_UINT32;
    } else {
        uint32_t newpos;
        if (this->file.seek(pos)) {
            newpos = pos;
        } else {
            newpos = this->position();
        }

        return newpos;
    }
}
