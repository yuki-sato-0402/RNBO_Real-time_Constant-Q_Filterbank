/*******************************************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER $200k in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below $200k annual revenue or funding.

For entities with OVER $200k in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing@cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
*******************************************************************************************************************/

#ifdef RNBO_LIB_PREFIX
#define STR_IMPL(A) #A
#define STR(A) STR_IMPL(A)
#define RNBO_LIB_INCLUDE(X) STR(RNBO_LIB_PREFIX/X)
#else
#define RNBO_LIB_INCLUDE(X) #X
#endif // RNBO_LIB_PREFIX
#ifdef RNBO_INJECTPLATFORM
#define RNBO_USECUSTOMPLATFORM
#include RNBO_INJECTPLATFORM
#endif // RNBO_INJECTPLATFORM

#include RNBO_LIB_INCLUDE(RNBO_Common.h)
#include RNBO_LIB_INCLUDE(RNBO_AudioSignal.h)

namespace RNBO {


#define trunc(x) ((Int)(x))
#define autoref auto&

#if defined(__GNUC__) || defined(__clang__)
    #define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

template <class ENGINE = INTERNALENGINE> class rnbomatic : public PatcherInterfaceImpl {

friend class EngineCore;
friend class Engine;
friend class MinimalEngine<>;
public:

rnbomatic()
: _internalEngine(this)
{
}

~rnbomatic()
{
    deallocateSignals();
}

Index getNumMidiInputPorts() const {
    return 1;
}

void processMidiEvent(MillisecondTime time, int port, ConstByteArray data, Index length) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->ctlin_01_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_02_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_03_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
    this->ctlin_04_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
}

Index getNumMidiOutputPorts() const {
    return 0;
}

void process(
    const SampleValue * const* inputs,
    Index numInputs,
    SampleValue * const* outputs,
    Index numOutputs,
    Index n
) {
    this->vs = n;
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
    SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
    SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
    const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
    const SampleValue * in2 = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);

    this->gen_01_perform(
        in1,
        in2,
        this->gen_01_Q_factor,
        this->gen_01_overtoneThreshold,
        this->gen_01_polyTolerance,
        this->gen_01_noiseFloor,
        out1,
        out2,
        this->signals[0],
        this->signals[1],
        n
    );

    this->numbertilde_01_perform(this->signals[0], this->dummyBuffer, n);
    this->numbertilde_02_perform(this->signals[1], this->dummyBuffer, n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
        Index i;

        for (i = 0; i < 2; i++) {
            this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
        }

        this->globaltransport_tempo = resizeSignal(this->globaltransport_tempo, this->maxvs, maxBlockSize);
        this->globaltransport_state = resizeSignal(this->globaltransport_state, this->maxvs, maxBlockSize);
        this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
        this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
        this->didAllocateSignals = true;
    }

    const bool sampleRateChanged = sampleRate != this->sr;
    const bool maxvsChanged = maxBlockSize != this->maxvs;
    const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

    if (sampleRateChanged || maxvsChanged) {
        this->vs = maxBlockSize;
        this->maxvs = maxBlockSize;
        this->sr = sampleRate;
        this->invsr = 1 / sampleRate;
    }

    this->numbertilde_01_dspsetup(forceDSPSetup);
    this->numbertilde_02_dspsetup(forceDSPSetup);
    this->data_01_dspsetup(forceDSPSetup);
    this->globaltransport_dspsetup(forceDSPSetup);

    if (sampleRateChanged)
        this->onSampleRateChanged(sampleRate);
}

number msToSamps(MillisecondTime ms, number sampleRate) {
    return ms * sampleRate * 0.001;
}

MillisecondTime sampsToMs(SampleIndex samps) {
    return samps * (this->invsr * 1000);
}

Index getNumInputChannels() const {
    return 2;
}

Index getNumOutputChannels() const {
    return 2;
}

DataRef* getDataRef(DataRefIndex index)  {
    switch (index) {
    case 0:
        {
        return addressOf(this->OutputBuff);
        break;
        }
    case 1:
        {
        return addressOf(this->countBuff);
        break;
        }
    case 2:
        {
        return addressOf(this->phaseBuff);
        break;
        }
    case 3:
        {
        return addressOf(this->sumCosBuff);
        break;
        }
    case 4:
        {
        return addressOf(this->sumSinBuff);
        break;
        }
    case 5:
        {
        return addressOf(this->currentData);
        break;
        }
    case 6:
        {
        return addressOf(this->workingData);
        break;
        }
    case 7:
        {
        return addressOf(this->RNBODefaultMtofLookupTable256);
        break;
        }
    default:
        {
        return nullptr;
        }
    }
}

DataRefIndex getNumDataRefs() const {
    return 8;
}

void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (index == 0) {
        this->gen_01_outputBuff = reInitDataView(this->gen_01_outputBuff, this->OutputBuff);
        this->data_01_buffer = reInitDataView(this->data_01_buffer, this->OutputBuff);
        this->data_01_bufferUpdated();
    }

    if (index == 1) {
        this->gen_01_countBuff = reInitDataView(this->gen_01_countBuff, this->countBuff);
    }

    if (index == 2) {
        this->gen_01_phaseBuff = reInitDataView(this->gen_01_phaseBuff, this->phaseBuff);
    }

    if (index == 3) {
        this->gen_01_sumCosBuff = reInitDataView(this->gen_01_sumCosBuff, this->sumCosBuff);
    }

    if (index == 4) {
        this->gen_01_sumSinBuff = reInitDataView(this->gen_01_sumSinBuff, this->sumSinBuff);
    }

    if (index == 5) {
        this->gen_01_currentData = reInitDataView(this->gen_01_currentData, this->currentData);
    }

    if (index == 6) {
        this->gen_01_workingData = reInitDataView(this->gen_01_workingData, this->workingData);
    }

    if (index == 7) {
        this->gen_01_mtof_2_buffer = reInitDataView(this->gen_01_mtof_2_buffer, this->RNBODefaultMtofLookupTable256);
    }
}

void initialize() {
    RNBO_ASSERT(!this->_isInitialized);

    this->OutputBuff = initDataRef(
        this->OutputBuff,
        this->dataRefStrings->name0,
        false,
        this->dataRefStrings->file0,
        this->dataRefStrings->tag0
    );

    this->countBuff = initDataRef(
        this->countBuff,
        this->dataRefStrings->name1,
        true,
        this->dataRefStrings->file1,
        this->dataRefStrings->tag1
    );

    this->phaseBuff = initDataRef(
        this->phaseBuff,
        this->dataRefStrings->name2,
        true,
        this->dataRefStrings->file2,
        this->dataRefStrings->tag2
    );

    this->sumCosBuff = initDataRef(
        this->sumCosBuff,
        this->dataRefStrings->name3,
        true,
        this->dataRefStrings->file3,
        this->dataRefStrings->tag3
    );

    this->sumSinBuff = initDataRef(
        this->sumSinBuff,
        this->dataRefStrings->name4,
        true,
        this->dataRefStrings->file4,
        this->dataRefStrings->tag4
    );

    this->currentData = initDataRef(
        this->currentData,
        this->dataRefStrings->name5,
        true,
        this->dataRefStrings->file5,
        this->dataRefStrings->tag5
    );

    this->workingData = initDataRef(
        this->workingData,
        this->dataRefStrings->name6,
        true,
        this->dataRefStrings->file6,
        this->dataRefStrings->tag6
    );

    this->RNBODefaultMtofLookupTable256 = initDataRef(
        this->RNBODefaultMtofLookupTable256,
        this->dataRefStrings->name7,
        true,
        this->dataRefStrings->file7,
        this->dataRefStrings->tag7
    );

    this->assign_defaults();
    this->applyState();
    this->OutputBuff->setIndex(0);
    this->gen_01_outputBuff = new Float32Buffer(this->OutputBuff);
    this->data_01_buffer = new Float32Buffer(this->OutputBuff);
    this->countBuff->setIndex(1);
    this->gen_01_countBuff = new SampleBuffer(this->countBuff);
    this->phaseBuff->setIndex(2);
    this->gen_01_phaseBuff = new SampleBuffer(this->phaseBuff);
    this->sumCosBuff->setIndex(3);
    this->gen_01_sumCosBuff = new SampleBuffer(this->sumCosBuff);
    this->sumSinBuff->setIndex(4);
    this->gen_01_sumSinBuff = new SampleBuffer(this->sumSinBuff);
    this->currentData->setIndex(5);
    this->gen_01_currentData = new SampleBuffer(this->currentData);
    this->workingData->setIndex(6);
    this->gen_01_workingData = new SampleBuffer(this->workingData);
    this->RNBODefaultMtofLookupTable256->setIndex(7);
    this->gen_01_mtof_2_buffer = new SampleBuffer(this->RNBODefaultMtofLookupTable256);
    this->initializeObjects();
    this->allocateDataRefs();
    this->startup();
    this->_isInitialized = true;
}

void getPreset(PatcherStateInterface& preset) {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    preset["__presetid"] = "rnbo";
    this->param_01_getPresetValue(getSubState(preset, "Q_factor"));
    this->param_02_getPresetValue(getSubState(preset, "polyTolerance"));
    this->param_03_getPresetValue(getSubState(preset, "overtoneThreshold"));
    this->param_04_getPresetValue(getSubState(preset, "noiseFloor"));
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->param_01_setPresetValue(getSubState(preset, "Q_factor"));
    this->param_02_setPresetValue(getSubState(preset, "polyTolerance"));
    this->param_03_setPresetValue(getSubState(preset, "overtoneThreshold"));
    this->param_04_setPresetValue(getSubState(preset, "noiseFloor"));
}

void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case 0:
        {
        this->param_01_value_set(v);
        break;
        }
    case 1:
        {
        this->param_02_value_set(v);
        break;
        }
    case 2:
        {
        this->param_03_value_set(v);
        break;
        }
    case 3:
        {
        this->param_04_value_set(v);
        break;
        }
    }
}

void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValue(index, value, time);
}

void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
    this->setParameterValue(index, this->getParameterValue(index), time);
}

void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValueNormalized(index, value, time);
}

ParameterValue getParameterValue(ParameterIndex index)  {
    switch (index) {
    case 0:
        {
        return this->param_01_value;
        }
    case 1:
        {
        return this->param_02_value;
        }
    case 2:
        {
        return this->param_03_value;
        }
    case 3:
        {
        return this->param_04_value;
        }
    default:
        {
        return 0;
        }
    }
}

ParameterIndex getNumSignalInParameters() const {
    return 0;
}

ParameterIndex getNumSignalOutParameters() const {
    return 0;
}

ParameterIndex getNumParameters() const {
    return 4;
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "Q_factor";
        }
    case 1:
        {
        return "polyTolerance";
        }
    case 2:
        {
        return "overtoneThreshold";
        }
    case 3:
        {
        return "noiseFloor";
        }
    default:
        {
        return "bogus";
        }
    }
}

ConstCharPointer getParameterId(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "Q_factor";
        }
    case 1:
        {
        return "polyTolerance";
        }
    case 2:
        {
        return "overtoneThreshold";
        }
    case 3:
        {
        return "noiseFloor";
        }
    default:
        {
        return "bogus";
        }
    }
}

void getParameterInfo(ParameterIndex index, ParameterInfo * info) const {
    {
        switch (index) {
        case 0:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 1.5;
            info->min = 1;
            info->max = 2;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 1:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 20;
            info->min = 10;
            info->max = 50;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 2:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 50;
            info->min = 10;
            info->max = 80;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 3:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 5;
            info->min = 0;
            info->max = 30;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        }
    }
}

ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
    if (steps == 1) {
        if (normalizedValue > 0) {
            normalizedValue = 1.;
        }
    } else {
        ParameterValue oneStep = (number)1. / (steps - 1);
        ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
        normalizedValue = numberOfSteps * oneStep;
    }

    return normalizedValue;
}

ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 3:
        {
        {
            value = (value < 0 ? 0 : (value > 30 ? 30 : value));
            ParameterValue normalizedValue = (value - 0) / (30 - 0);
            return normalizedValue;
        }
        }
    case 0:
        {
        {
            value = (value < 1 ? 1 : (value > 2 ? 2 : value));
            ParameterValue normalizedValue = (value - 1) / (2 - 1);
            return normalizedValue;
        }
        }
    case 1:
        {
        {
            value = (value < 10 ? 10 : (value > 50 ? 50 : value));
            ParameterValue normalizedValue = (value - 10) / (50 - 10);
            return normalizedValue;
        }
        }
    case 2:
        {
        {
            value = (value < 10 ? 10 : (value > 80 ? 80 : value));
            ParameterValue normalizedValue = (value - 10) / (80 - 10);
            return normalizedValue;
        }
        }
    default:
        {
        return value;
        }
    }
}

ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

    switch (index) {
    case 3:
        {
        {
            {
                return 0 + value * (30 - 0);
            }
        }
        }
    case 0:
        {
        {
            {
                return 1 + value * (2 - 1);
            }
        }
        }
    case 1:
        {
        {
            {
                return 10 + value * (50 - 10);
            }
        }
        }
    case 2:
        {
        {
            {
                return 10 + value * (80 - 10);
            }
        }
        }
    default:
        {
        return value;
        }
    }
}

ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 0:
        {
        return this->param_01_value_constrain(value);
        }
    case 1:
        {
        return this->param_02_value_constrain(value);
        }
    case 2:
        {
        return this->param_03_value_constrain(value);
        }
    case 3:
        {
        return this->param_04_value_constrain(value);
        }
    default:
        {
        return value;
        }
    }
}

void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("sig"):
        {
        if (TAG("number~_obj-15") == objectId)
            this->numbertilde_01_sig_number_set(payload);

        if (TAG("number~_obj-10") == objectId)
            this->numbertilde_02_sig_number_set(payload);

        break;
        }
    case TAG("mode"):
        {
        if (TAG("number~_obj-15") == objectId)
            this->numbertilde_01_mode_set(payload);

        if (TAG("number~_obj-10") == objectId)
            this->numbertilde_02_mode_set(payload);

        break;
        }
    }
}

void processListMessage(
    MessageTag tag,
    MessageTag objectId,
    MillisecondTime time,
    const list& payload
) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("sig"):
        {
        if (TAG("number~_obj-15") == objectId)
            this->numbertilde_01_sig_list_set(payload);

        if (TAG("number~_obj-10") == objectId)
            this->numbertilde_02_sig_list_set(payload);

        break;
        }
    }
}

void processBangMessage(MessageTag , MessageTag , MillisecondTime ) {}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {
    case TAG("monitor"):
        {
        return "monitor";
        }
    case TAG("number~_obj-15"):
        {
        return "number~_obj-15";
        }
    case TAG("assign"):
        {
        return "assign";
        }
    case TAG("setup"):
        {
        return "setup";
        }
    case TAG("number~_obj-10"):
        {
        return "number~_obj-10";
        }
    case TAG("sig"):
        {
        return "sig";
        }
    case TAG("mode"):
        {
        return "mode";
        }
    }

    return "";
}

MessageIndex getNumMessages() const {
    return 0;
}

const MessageInfo& getMessageInfo(MessageIndex index) const {
    switch (index) {

    }

    return NullMessageInfo;
}

protected:

		
void advanceTime(EXTERNALENGINE*) {}
void advanceTime(INTERNALENGINE*) {
	_internalEngine.advanceTime(sampstoms(this->vs));
}

void processInternalEvents(MillisecondTime time) {
	_internalEngine.processEventsUntil(time);
}

void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
	if (time == TimeNow) time = getPatcherTime();
	processInternalEvents(inProcess ? time + sampsToMs(this->vs) : time);
	updateTime(time, (EXTERNALENGINE*)nullptr);
}

rnbomatic* operator->() {
    return this;
}
const rnbomatic* operator->() const {
    return this;
}
rnbomatic* getTopLevelPatcher() {
    return this;
}

void cancelClockEvents()
{
    getEngine()->flushClockEvents(this, 2098551528, false);
    getEngine()->flushClockEvents(this, -1494586265, false);
}

template<typename LISTTYPE = list> void listquicksort(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    if (l < h) {
        Int p = (Int)(this->listpartition(arr, sortindices, l, h, ascending));
        this->listquicksort(arr, sortindices, l, p - 1, ascending);
        this->listquicksort(arr, sortindices, p + 1, h, ascending);
    }
}

template<typename LISTTYPE = list> Int listpartition(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    number x = arr[(Index)h];
    Int i = (Int)(l - 1);

    for (Int j = (Int)(l); j <= h - 1; j++) {
        bool asc = (bool)((bool)(ascending) && arr[(Index)j] <= x);
        bool desc = (bool)((bool)(!(bool)(ascending)) && arr[(Index)j] >= x);

        if ((bool)(asc) || (bool)(desc)) {
            i++;
            this->listswapelements(arr, i, j);
            this->listswapelements(sortindices, i, j);
        }
    }

    i++;
    this->listswapelements(arr, i, h);
    this->listswapelements(sortindices, i, h);
    return i;
}

template<typename LISTTYPE = list> void listswapelements(LISTTYPE& arr, Int a, Int b) {
    auto tmp = arr[(Index)a];
    arr[(Index)a] = arr[(Index)b];
    arr[(Index)b] = tmp;
}

inline number linearinterp(number frac, number x, number y) {
    return x + (y - x) * frac;
}

inline number safemod(number f, number m) {
    if (m != 0) {
        Int f_trunc = (Int)(trunc(f));
        Int m_trunc = (Int)(trunc(m));

        if (f == f_trunc && m == m_trunc) {
            f = f_trunc % m_trunc;
        } else {
            if (m < 0) {
                m = -m;
            }

            if (f >= m) {
                if (f >= m * 2.0) {
                    number d = f / m;
                    Int i = (Int)(trunc(d));
                    d = d - i;
                    f = d * m;
                } else {
                    f -= m;
                }
            } else if (f <= -m) {
                if (f <= -m * 2.0) {
                    number d = f / m;
                    Int i = (Int)(trunc(d));
                    d = d - i;
                    f = d * m;
                } else {
                    f += m;
                }
            }
        }
    } else {
        f = 0.0;
    }

    return f;
}

number wrap(number x, number low, number high) {
    number lo;
    number hi;

    if (low == high)
        return low;

    if (low > high) {
        hi = low;
        lo = high;
    } else {
        lo = low;
        hi = high;
    }

    number range = hi - lo;

    if (x >= lo && x < hi)
        return x;

    if (range <= 0.000000001)
        return lo;

    Int numWraps = (Int)(trunc((x - lo) / range));
    numWraps = numWraps - ((x < lo ? 1 : 0));
    number result = x - range * numWraps;

    if (result >= hi)
        return result - range;
    else
        return result;
}

inline number cubicinterp(number a, number w, number x, number y, number z) {
    number a1 = 1. + a;
    number aa = a * a1;
    number b = 1. - a;
    number b1 = 2. - a;
    number bb = b * b1;
    number fw = -.1666667 * bb * a;
    number fx = .5 * bb * a1;
    number fy = .5 * aa * b1;
    number fz = -.1666667 * aa * b;
    return w * fw + x * fx + y * fy + z * fz;
}

inline number fastcubicinterp(number a, number w, number x, number y, number z) {
    number a2 = a * a;
    number f0 = z - y - w + x;
    number f1 = w - x - f0;
    number f2 = y - w;
    number f3 = x;
    return f0 * a * a2 + f1 * a2 + f2 * a + f3;
}

inline number splineinterp(number a, number w, number x, number y, number z) {
    number a2 = a * a;
    number f0 = -0.5 * w + 1.5 * x - 1.5 * y + 0.5 * z;
    number f1 = w - 2.5 * x + 2 * y - 0.5 * z;
    number f2 = -0.5 * w + 0.5 * y;
    return f0 * a * a2 + f1 * a2 + f2 * a + x;
}

inline number spline6interp(number a, number y0, number y1, number y2, number y3, number y4, number y5) {
    number ym2py2 = y0 + y4;
    number ym1py1 = y1 + y3;
    number y2mym2 = y4 - y0;
    number y1mym1 = y3 - y1;
    number sixthym1py1 = (number)1 / (number)6.0 * ym1py1;
    number c0 = (number)1 / (number)120.0 * ym2py2 + (number)13 / (number)60.0 * ym1py1 + (number)11 / (number)20.0 * y2;
    number c1 = (number)1 / (number)24.0 * y2mym2 + (number)5 / (number)12.0 * y1mym1;
    number c2 = (number)1 / (number)12.0 * ym2py2 + sixthym1py1 - (number)1 / (number)2.0 * y2;
    number c3 = (number)1 / (number)12.0 * y2mym2 - (number)1 / (number)6.0 * y1mym1;
    number c4 = (number)1 / (number)24.0 * ym2py2 - sixthym1py1 + (number)1 / (number)4.0 * y2;
    number c5 = (number)1 / (number)120.0 * (y5 - y0) + (number)1 / (number)24.0 * (y1 - y4) + (number)1 / (number)12.0 * (y3 - y2);
    return ((((c5 * a + c4) * a + c3) * a + c2) * a + c1) * a + c0;
}

inline number cosT8(number r) {
    number t84 = 56.0;
    number t83 = 1680.0;
    number t82 = 20160.0;
    number t81 = 2.4801587302e-05;
    number t73 = 42.0;
    number t72 = 840.0;
    number t71 = 1.9841269841e-04;

    if (r < 0.785398163397448309615660845819875721 && r > -0.785398163397448309615660845819875721) {
        number rr = r * r;
        return 1.0 - rr * t81 * (t82 - rr * (t83 - rr * (t84 - rr)));
    } else if (r > 0.0) {
        r -= 1.57079632679489661923132169163975144;
        number rr = r * r;
        return -r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
    } else {
        r += 1.57079632679489661923132169163975144;
        number rr = r * r;
        return r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
    }
}

inline number cosineinterp(number frac, number x, number y) {
    number a2 = (1.0 - this->cosT8(frac * 3.14159265358979323846)) / (number)2.0;
    return x * (1.0 - a2) + y * a2;
}

template<typename BUFFERTYPE> array<SampleValue, 1 + 1> peek_default(BUFFERTYPE& buffer, SampleValue sampleIndex, Index channelOffset) {
    number bufferSize = buffer->getSize();
    const Index bufferChannels = (const Index)(buffer->getChannels());
    constexpr int ___N2 = 1 + 1;
    array<SampleValue, ___N2> out = FIXEDSIZEARRAYINIT(1 + 1);

    if (bufferSize == 0 || (5 == 5 && (sampleIndex < 0 || sampleIndex >= bufferSize))) {
        return out;
    } else {
        for (Index channel = 0; channel < 1; channel++) {
            Index channelIndex = (Index)(channel + channelOffset);

            {
                if (channelIndex >= bufferChannels || channelIndex < 0) {
                    out[(Index)channel] = 0;
                    continue;
                }
            }

            SampleValue bufferreadsample_result;

            {
                auto& __buffer = buffer;

                if (sampleIndex < 0.0) {
                    bufferreadsample_result = 0.0;
                }

                SampleIndex truncIndex = (SampleIndex)(trunc(sampleIndex));

                {
                    bufferreadsample_result = __buffer->getSample(channelIndex, truncIndex);
                }
            }

            out[(Index)channel] = bufferreadsample_result;
        }

        out[1] = sampleIndex;
        return out;
    }
}

template<typename BUFFERTYPE> void poke_default(
    BUFFERTYPE& buffer,
    SampleValue value,
    SampleValue sampleIndex,
    Int channel,
    number overdub
) {
    number bufferSize = buffer->getSize();
    const Index bufferChannels = (const Index)(buffer->getChannels());

    if (bufferSize > 0 && (5 != 5 || (sampleIndex >= 0 && sampleIndex < bufferSize)) && (5 != 5 || (channel >= 0 && channel < bufferChannels))) {
        if (overdub != 0) {
            number currentValue = buffer->getSample(channel, sampleIndex);

            {
                value = value + currentValue * overdub;
            }
        }

        buffer->setSample(channel, sampleIndex, value);
        buffer->setTouched(true);
    }
}

template<typename BUFFERTYPE> number dim(BUFFERTYPE& buffer) {
    return buffer->getSize();
}

template<typename BUFFERTYPE> array<SampleValue, 1 + 1> peek_boundmode_ignore(BUFFERTYPE& buffer, SampleValue sampleIndex, Index channelOffset) {
    number bufferSize = buffer->getSize();
    const Index bufferChannels = (const Index)(buffer->getChannels());
    constexpr int ___N2 = 1 + 1;
    array<SampleValue, ___N2> out = FIXEDSIZEARRAYINIT(1 + 1);

    if (bufferSize == 0 || (5 == 5 && (sampleIndex < 0 || sampleIndex >= bufferSize))) {
        return out;
    } else {
        for (Index channel = 0; channel < 1; channel++) {
            Index channelIndex = (Index)(channel + channelOffset);

            {
                if (channelIndex >= bufferChannels || channelIndex < 0) {
                    out[(Index)channel] = 0;
                    continue;
                }
            }

            SampleValue bufferreadsample_result;

            {
                auto& __buffer = buffer;

                if (sampleIndex < 0.0) {
                    bufferreadsample_result = 0.0;
                }

                SampleIndex truncIndex = (SampleIndex)(trunc(sampleIndex));

                {
                    bufferreadsample_result = __buffer->getSample(channelIndex, truncIndex);
                }
            }

            out[(Index)channel] = bufferreadsample_result;
        }

        out[1] = sampleIndex;
        return out;
    }
}

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
}

number fromnormalized(Index index, number normalizedValue) {
    return this->convertFromNormalizedParameterValue(index, normalizedValue);
}

number maximum(number x, number y) {
    return (x < y ? y : x);
}

MillisecondTime sampstoms(number samps) {
    return samps * 1000 / this->sr;
}

void param_01_value_set(number v) {
    v = this->param_01_value_constrain(v);
    this->param_01_value = v;
    this->sendParameter(0, false);

    if (this->param_01_value != this->param_01_lastValue) {
        this->getEngine()->presetTouched();
        this->param_01_lastValue = this->param_01_value;
    }

    this->gen_01_Q_factor_set(v);
}

void param_02_value_set(number v) {
    v = this->param_02_value_constrain(v);
    this->param_02_value = v;
    this->sendParameter(1, false);

    if (this->param_02_value != this->param_02_lastValue) {
        this->getEngine()->presetTouched();
        this->param_02_lastValue = this->param_02_value;
    }

    this->gen_01_polyTolerance_set(v);
}

void param_03_value_set(number v) {
    v = this->param_03_value_constrain(v);
    this->param_03_value = v;
    this->sendParameter(2, false);

    if (this->param_03_value != this->param_03_lastValue) {
        this->getEngine()->presetTouched();
        this->param_03_lastValue = this->param_03_value;
    }

    this->gen_01_overtoneThreshold_set(v);
}

void param_04_value_set(number v) {
    v = this->param_04_value_constrain(v);
    this->param_04_value = v;
    this->sendParameter(3, false);

    if (this->param_04_value != this->param_04_lastValue) {
        this->getEngine()->presetTouched();
        this->param_04_lastValue = this->param_04_value;
    }

    this->gen_01_noiseFloor_set(v);
}

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

void numbertilde_01_sig_number_set(number v) {
    this->numbertilde_01_outValue = v;
}

template<typename LISTTYPE> void numbertilde_01_sig_list_set(const LISTTYPE& v) {
    this->numbertilde_01_outValue = v[0];
}

void numbertilde_01_mode_set(number v) {
    if (v == 1) {
        this->numbertilde_01_currentMode = 0;
    } else if (v == 2) {
        this->numbertilde_01_currentMode = 1;
    }
}

void numbertilde_02_sig_number_set(number v) {
    this->numbertilde_02_outValue = v;
}

template<typename LISTTYPE> void numbertilde_02_sig_list_set(const LISTTYPE& v) {
    this->numbertilde_02_outValue = v[0];
}

void numbertilde_02_mode_set(number v) {
    if (v == 1) {
        this->numbertilde_02_currentMode = 0;
    } else if (v == 2) {
        this->numbertilde_02_currentMode = 1;
    }
}

void numbertilde_01_value_set(number ) {}

void numbertilde_02_value_set(number ) {}

void deallocateSignals() {
    Index i;

    for (i = 0; i < 2; i++) {
        this->signals[i] = freeSignal(this->signals[i]);
    }

    this->globaltransport_tempo = freeSignal(this->globaltransport_tempo);
    this->globaltransport_state = freeSignal(this->globaltransport_state);
    this->zeroBuffer = freeSignal(this->zeroBuffer);
    this->dummyBuffer = freeSignal(this->dummyBuffer);
}

Index getMaxBlockSize() const {
    return this->maxvs;
}

number getSampleRate() const {
    return this->sr;
}

bool hasFixedVectorSize() const {
    return false;
}

void setProbingTarget(MessageTag ) {}

void fillRNBODefaultMtofLookupTable256(DataRef& ref) {
    SampleBuffer buffer(ref);
    number bufsize = buffer->getSize();

    for (Index i = 0; i < bufsize; i++) {
        number midivalue = -256. + (number)512. / (bufsize - 1) * i;
        buffer[i] = rnbo_exp(.057762265 * (midivalue - 69.0));
    }
}

void fillDataRef(DataRefIndex index, DataRef& ref) {
    switch (index) {
    case 7:
        {
        this->fillRNBODefaultMtofLookupTable256(ref);
        break;
        }
    }
}

void zeroDataRef(DataRef& ref) {
    ref->setZero();
}

void allocateDataRefs() {
    this->data_01_buffer->requestSize(88, 1);
    this->data_01_buffer->setSampleRate(this->sr);
    this->gen_01_countBuff->requestSize(88, 2);
    this->gen_01_countBuff->setSampleRate(this->sr);
    this->gen_01_phaseBuff->requestSize(88, 2);
    this->gen_01_phaseBuff->setSampleRate(this->sr);
    this->gen_01_sumCosBuff->requestSize(88, 2);
    this->gen_01_sumCosBuff->setSampleRate(this->sr);
    this->gen_01_sumSinBuff->requestSize(88, 2);
    this->gen_01_sumSinBuff->setSampleRate(this->sr);
    this->gen_01_currentData->requestSize(6, 2);
    this->gen_01_currentData->setSampleRate(this->sr);
    this->gen_01_workingData->requestSize(6, 2);
    this->gen_01_workingData->setSampleRate(this->sr);
    this->gen_01_mtof_2_buffer->requestSize(65536, 1);
    this->gen_01_mtof_2_buffer->setSampleRate(this->sr);
    this->gen_01_outputBuff = this->gen_01_outputBuff->allocateIfNeeded();
    this->data_01_buffer = this->data_01_buffer->allocateIfNeeded();

    if (this->OutputBuff->hasRequestedSize()) {
        if (this->OutputBuff->wantsFill())
            this->zeroDataRef(this->OutputBuff);

        this->getEngine()->sendDataRefUpdated(0);
    }

    this->gen_01_countBuff = this->gen_01_countBuff->allocateIfNeeded();

    if (this->countBuff->hasRequestedSize()) {
        if (this->countBuff->wantsFill())
            this->zeroDataRef(this->countBuff);

        this->getEngine()->sendDataRefUpdated(1);
    }

    this->gen_01_phaseBuff = this->gen_01_phaseBuff->allocateIfNeeded();

    if (this->phaseBuff->hasRequestedSize()) {
        if (this->phaseBuff->wantsFill())
            this->zeroDataRef(this->phaseBuff);

        this->getEngine()->sendDataRefUpdated(2);
    }

    this->gen_01_sumCosBuff = this->gen_01_sumCosBuff->allocateIfNeeded();

    if (this->sumCosBuff->hasRequestedSize()) {
        if (this->sumCosBuff->wantsFill())
            this->zeroDataRef(this->sumCosBuff);

        this->getEngine()->sendDataRefUpdated(3);
    }

    this->gen_01_sumSinBuff = this->gen_01_sumSinBuff->allocateIfNeeded();

    if (this->sumSinBuff->hasRequestedSize()) {
        if (this->sumSinBuff->wantsFill())
            this->zeroDataRef(this->sumSinBuff);

        this->getEngine()->sendDataRefUpdated(4);
    }

    this->gen_01_currentData = this->gen_01_currentData->allocateIfNeeded();

    if (this->currentData->hasRequestedSize()) {
        if (this->currentData->wantsFill())
            this->zeroDataRef(this->currentData);

        this->getEngine()->sendDataRefUpdated(5);
    }

    this->gen_01_workingData = this->gen_01_workingData->allocateIfNeeded();

    if (this->workingData->hasRequestedSize()) {
        if (this->workingData->wantsFill())
            this->zeroDataRef(this->workingData);

        this->getEngine()->sendDataRefUpdated(6);
    }

    this->gen_01_mtof_2_buffer = this->gen_01_mtof_2_buffer->allocateIfNeeded();

    if (this->RNBODefaultMtofLookupTable256->hasRequestedSize()) {
        if (this->RNBODefaultMtofLookupTable256->wantsFill())
            this->fillRNBODefaultMtofLookupTable256(this->RNBODefaultMtofLookupTable256);

        this->getEngine()->sendDataRefUpdated(7);
    }
}

void initializeObjects() {
    this->numbertilde_01_init();
    this->numbertilde_02_init();
    this->data_01_init();
}

Index getIsMuted()  {
    return this->isMuted;
}

void setIsMuted(Index v)  {
    this->isMuted = v;
}

void onSampleRateChanged(double ) {}

void extractState(PatcherStateInterface& ) {}

void applyState() {}

void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
    RNBO_UNUSED(hasValue);
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case 2098551528:
        {
        this->numbertilde_01_value_set(value);
        break;
        }
    case -1494586265:
        {
        this->numbertilde_02_value_set(value);
        break;
        }
    }
}

void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}

void processOutletEvent(
    EngineLink* sender,
    OutletIndex index,
    ParameterValue value,
    MillisecondTime time
) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->processOutletAtCurrentTime(sender, index, value);
}

void sendOutlet(OutletIndex index, ParameterValue value) {
    this->getEngine()->sendOutlet(this, index, value);
}

void startup() {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);

    {
        this->scheduleParamInit(0, 0);
    }

    {
        this->scheduleParamInit(1, 0);
    }

    {
        this->scheduleParamInit(2, 0);
    }

    {
        this->scheduleParamInit(3, 0);
    }

    this->processParamInitEvents();
}

number param_01_value_constrain(number v) const {
    v = (v > 2 ? 2 : (v < 1 ? 1 : v));
    return v;
}

void gen_01_Q_factor_set(number v) {
    this->gen_01_Q_factor = v;
}

number param_02_value_constrain(number v) const {
    v = (v > 50 ? 50 : (v < 10 ? 10 : v));
    return v;
}

void gen_01_polyTolerance_set(number v) {
    this->gen_01_polyTolerance = v;
}

number param_03_value_constrain(number v) const {
    v = (v > 80 ? 80 : (v < 10 ? 10 : v));
    return v;
}

void gen_01_overtoneThreshold_set(number v) {
    this->gen_01_overtoneThreshold = v;
}

number param_04_value_constrain(number v) const {
    v = (v > 30 ? 30 : (v < 0 ? 0 : v));
    return v;
}

void gen_01_noiseFloor_set(number v) {
    this->gen_01_noiseFloor = v;
}

void ctlin_01_outchannel_set(number ) {}

void ctlin_01_outcontroller_set(number ) {}

void fromnormalized_01_output_set(number v) {
    this->param_01_value_set(v);
}

void fromnormalized_01_input_set(number v) {
    this->fromnormalized_01_output_set(this->fromnormalized(0, v));
}

void expr_01_out1_set(number v) {
    this->expr_01_out1 = v;
    this->fromnormalized_01_input_set(this->expr_01_out1);
}

void expr_01_in1_set(number in1) {
    this->expr_01_in1 = in1;
    this->expr_01_out1_set(this->expr_01_in1 * this->expr_01_in2);//#map:expr_01:1
}

void ctlin_01_value_set(number v) {
    this->expr_01_in1_set(v);
}

void ctlin_01_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_01_channel || this->ctlin_01_channel == -1) && (data[1] == this->ctlin_01_controller || this->ctlin_01_controller == -1)) {
        this->ctlin_01_outchannel_set(channel);
        this->ctlin_01_outcontroller_set(data[1]);
        this->ctlin_01_value_set(data[2]);
        this->ctlin_01_status = 0;
    }
}

void ctlin_02_outchannel_set(number ) {}

void ctlin_02_outcontroller_set(number ) {}

void fromnormalized_02_output_set(number v) {
    this->param_02_value_set(v);
}

void fromnormalized_02_input_set(number v) {
    this->fromnormalized_02_output_set(this->fromnormalized(1, v));
}

void expr_02_out1_set(number v) {
    this->expr_02_out1 = v;
    this->fromnormalized_02_input_set(this->expr_02_out1);
}

void expr_02_in1_set(number in1) {
    this->expr_02_in1 = in1;
    this->expr_02_out1_set(this->expr_02_in1 * this->expr_02_in2);//#map:expr_02:1
}

void ctlin_02_value_set(number v) {
    this->expr_02_in1_set(v);
}

void ctlin_02_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_02_channel || this->ctlin_02_channel == -1) && (data[1] == this->ctlin_02_controller || this->ctlin_02_controller == -1)) {
        this->ctlin_02_outchannel_set(channel);
        this->ctlin_02_outcontroller_set(data[1]);
        this->ctlin_02_value_set(data[2]);
        this->ctlin_02_status = 0;
    }
}

void ctlin_03_outchannel_set(number ) {}

void ctlin_03_outcontroller_set(number ) {}

void fromnormalized_03_output_set(number v) {
    this->param_03_value_set(v);
}

void fromnormalized_03_input_set(number v) {
    this->fromnormalized_03_output_set(this->fromnormalized(2, v));
}

void expr_03_out1_set(number v) {
    this->expr_03_out1 = v;
    this->fromnormalized_03_input_set(this->expr_03_out1);
}

void expr_03_in1_set(number in1) {
    this->expr_03_in1 = in1;
    this->expr_03_out1_set(this->expr_03_in1 * this->expr_03_in2);//#map:expr_03:1
}

void ctlin_03_value_set(number v) {
    this->expr_03_in1_set(v);
}

void ctlin_03_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_03_channel || this->ctlin_03_channel == -1) && (data[1] == this->ctlin_03_controller || this->ctlin_03_controller == -1)) {
        this->ctlin_03_outchannel_set(channel);
        this->ctlin_03_outcontroller_set(data[1]);
        this->ctlin_03_value_set(data[2]);
        this->ctlin_03_status = 0;
    }
}

void ctlin_04_outchannel_set(number ) {}

void ctlin_04_outcontroller_set(number ) {}

void fromnormalized_04_output_set(number v) {
    this->param_04_value_set(v);
}

void fromnormalized_04_input_set(number v) {
    this->fromnormalized_04_output_set(this->fromnormalized(3, v));
}

void expr_04_out1_set(number v) {
    this->expr_04_out1 = v;
    this->fromnormalized_04_input_set(this->expr_04_out1);
}

void expr_04_in1_set(number in1) {
    this->expr_04_in1 = in1;
    this->expr_04_out1_set(this->expr_04_in1 * this->expr_04_in2);//#map:expr_04:1
}

void ctlin_04_value_set(number v) {
    this->expr_04_in1_set(v);
}

void ctlin_04_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (status == 0xB0 && (channel == this->ctlin_04_channel || this->ctlin_04_channel == -1) && (data[1] == this->ctlin_04_controller || this->ctlin_04_controller == -1)) {
        this->ctlin_04_outchannel_set(channel);
        this->ctlin_04_outcontroller_set(data[1]);
        this->ctlin_04_value_set(data[2]);
        this->ctlin_04_status = 0;
    }
}

void gen_01_perform(
    const Sample * in1,
    const Sample * in2,
    number Q_factor,
    number overtoneThreshold,
    number polyTolerance,
    number noiseFloor,
    SampleValue * out1,
    SampleValue * out2,
    SampleValue * out3,
    SampleValue * out4,
    Index n
) {
    Index i0;

    for (i0 = 0; i0 < (Index)n; i0++) {
        number Q_0 = 16.817153745106 * Q_factor;
        number sumInput_1 = (in1[(Index)i0] + in2[(Index)i0]) * 0.5;

        for (number k = 0; k < 88; k = k + 1) {
            number freq_3 = this->gen_01_mtof_2_next(k + 21, 440);
            number winLength_4 = Q_0 * ((freq_3 == 0. ? 0. : this->sr / freq_3));
            number phaseStep_5 = (this->sr == 0. ? 0. : 6.28318530717958647692 * freq_3 / this->sr);
            number count1A = 0;
            number count2A = 0;
            auto result_6 = this->peek_default(this->gen_01_countBuff, k, 0);
            count2A = result_6[1];
            count1A = result_6[0];
            number count1B = 0;
            number count2B = 0;
            auto result_7 = this->peek_default(this->gen_01_countBuff, k, 1);
            count2B = result_7[1];
            count1B = result_7[0];

            number cosFuncA_8 = rnbo_cos(
                (winLength_4 - 1 == 0. ? 0. : 6.28318530717958647692 * count1A / (winLength_4 - 1))
            );

            number cosFuncB_9 = rnbo_cos(
                (winLength_4 - 1 == 0. ? 0. : 6.28318530717958647692 * (count1B - winLength_4 / (number)2) / (winLength_4 - 1))
            );

            number winFuncA_10 = 0.5 - 0.5 * cosFuncA_8;
            number winFuncB_11 = 0.5 - 0.5 * cosFuncB_9;
            number phase1A = 0;
            number phase2A = 0;
            auto result_12 = this->peek_default(this->gen_01_phaseBuff, k, 0);
            phase2A = result_12[1];
            phase1A = result_12[0];
            number phase1B = 0;
            number phase2B = 0;
            auto result_13 = this->peek_default(this->gen_01_phaseBuff, k, 1);
            phase2B = result_13[1];
            phase1B = result_13[0];
            number sumCos1A = 0;
            number sumCos2A = 0;
            auto result_14 = this->peek_default(this->gen_01_sumCosBuff, k, 0);
            sumCos2A = result_14[1];
            sumCos1A = result_14[0];

            this->poke_default(
                this->gen_01_sumCosBuff,
                sumCos1A + sumInput_1 * winFuncA_10 * rnbo_cos(phase1A),
                k,
                0,
                0
            );

            number sumCos1B = 0;
            number sumCos2B = 0;
            auto result_15 = this->peek_default(this->gen_01_sumCosBuff, k, 1);
            sumCos2B = result_15[1];
            sumCos1B = result_15[0];

            this->poke_default(
                this->gen_01_sumCosBuff,
                sumCos1B + sumInput_1 * winFuncB_11 * rnbo_cos(phase1B),
                k,
                1,
                0
            );

            number sumSin1A = 0;
            number sumSin2A = 0;
            auto result_16 = this->peek_default(this->gen_01_sumSinBuff, k, 0);
            sumSin2A = result_16[1];
            sumSin1A = result_16[0];

            this->poke_default(
                this->gen_01_sumSinBuff,
                sumSin1A + sumInput_1 * winFuncA_10 * rnbo_sin(phase1A),
                k,
                0,
                0
            );

            number sumSin1B = 0;
            number sumSin2B = 0;
            auto result_17 = this->peek_default(this->gen_01_sumSinBuff, k, 1);
            sumSin2B = result_17[1];
            sumSin1B = result_17[0];

            this->poke_default(
                this->gen_01_sumSinBuff,
                sumSin1B + sumInput_1 * winFuncB_11 * rnbo_sin(phase1B),
                k,
                1,
                0
            );

            if (count1A > winLength_4) {
                number cartopol_1 = 0;
                number cartopol_2 = 0;
                cartopol_1 = (sumCos1A * sumCos1A + sumSin1A * sumSin1A <= 0 ? 0 : rnbo_sqrt(sumCos1A * sumCos1A + sumSin1A * sumSin1A)), cartopol_2 = rnbo_atan2(sumSin1A, sumCos1A);
                number lastAmpA_18 = ((winLength_4 == 0. ? 0. : cartopol_1 / winLength_4)) * 4;

                if (lastAmpA_18 < noiseFloor * 0.01) {
                    lastAmpA_18 = 0;
                }

                this->poke_default(this->gen_01_outputBuff, lastAmpA_18, k, 0, 0);

                if (k == 0) {
                    {
                        auto WorkingData_20 = this->gen_01_workingData;
                        auto CurrentData_19 = this->gen_01_currentData;

                        for (number i = 0; i < this->dim(WorkingData_20); i = i + 1) {
                            number currentMaxAmp = 0;
                            number _ = 0;
                            auto result_21 = this->peek_default(WorkingData_20, i, 0);
                            _ = result_21[1];
                            currentMaxAmp = result_21[0];
                            this->poke_default(CurrentData_19, currentMaxAmp, i, 0, 0);
                            number currentMaxIndex = 0;
                            auto result_22 = this->peek_default(WorkingData_20, i, 1);
                            _ = result_22[1];
                            currentMaxIndex = result_22[0];
                            this->poke_default(CurrentData_19, currentMaxIndex, i, 1, 0);
                            this->poke_default(WorkingData_20, 0, i, 0, 0);
                            this->poke_default(WorkingData_20, 0, i, 1, 0);
                        }
                    }
                }

                {
                    auto WorkingData_25 = this->gen_01_workingData;
                    auto lastAmp_24 = lastAmpA_18;
                    auto k_23 = k;
                    number workingMaxAmp = 0;
                    number _ = 0;
                    auto result_26 = this->peek_default(WorkingData_25, 0, 0);
                    _ = result_26[1];
                    workingMaxAmp = result_26[0];
                    number workingMaxIndex = 0;
                    auto result_27 = this->peek_default(WorkingData_25, 0, 1);
                    _ = result_27[1];
                    workingMaxIndex = result_27[0];
                    number working2ndAmp = 0;
                    auto result_28 = this->peek_default(WorkingData_25, 1, 0);
                    _ = result_28[1];
                    working2ndAmp = result_28[0];
                    number working2ndIndex = 0;
                    auto result_29 = this->peek_default(WorkingData_25, 1, 1);
                    _ = result_29[1];
                    working2ndIndex = result_29[0];
                    number working3rdAmp = 0;
                    auto result_30 = this->peek_default(WorkingData_25, 2, 0);
                    _ = result_30[1];
                    working3rdAmp = result_30[0];
                    number working3rdIndex = 0;
                    auto result_31 = this->peek_default(WorkingData_25, 2, 1);
                    _ = result_31[1];
                    working3rdIndex = result_31[0];
                    number working4thAmp = 0;
                    auto result_32 = this->peek_default(WorkingData_25, 3, 0);
                    _ = result_32[1];
                    working4thAmp = result_32[0];
                    number working4thIndex = 0;
                    auto result_33 = this->peek_default(WorkingData_25, 3, 1);
                    _ = result_33[1];
                    working4thIndex = result_33[0];
                    number working5thAmp = 0;
                    auto result_34 = this->peek_default(WorkingData_25, 4, 0);
                    _ = result_34[1];
                    working5thAmp = result_34[0];
                    number working5thIndex = 0;
                    auto result_35 = this->peek_default(WorkingData_25, 4, 1);
                    _ = result_35[1];
                    working5thIndex = result_35[0];
                    number working6thAmp = 0;
                    auto result_36 = this->peek_default(WorkingData_25, 5, 0);
                    _ = result_36[1];
                    working6thAmp = result_36[0];
                    number working6thIndex = 0;
                    auto result_37 = this->peek_default(WorkingData_25, 5, 1);
                    _ = result_37[1];
                    working6thIndex = result_37[0];

                    if (k_23 == workingMaxIndex) {
                        this->poke_default(WorkingData_25, lastAmp_24, 0, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 0, 1, 0);
                    } else if (k_23 == working2ndIndex) {
                        this->poke_default(WorkingData_25, lastAmp_24, 1, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 1, 1, 0);
                    } else if (k_23 == working3rdIndex) {
                        this->poke_default(WorkingData_25, lastAmp_24, 2, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 2, 1, 0);
                    } else if (k_23 == working4thIndex) {
                        this->poke_default(WorkingData_25, lastAmp_24, 3, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 3, 1, 0);
                    } else if (k_23 == working5thIndex) {
                        this->poke_default(WorkingData_25, lastAmp_24, 4, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 4, 1, 0);
                    } else if (k_23 == working6thIndex) {
                        this->poke_default(WorkingData_25, lastAmp_24, 5, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 5, 1, 0);
                    } else if (lastAmp_24 > workingMaxAmp) {
                        this->poke_default(WorkingData_25, working5thAmp, 5, 0, 0);
                        this->poke_default(WorkingData_25, working5thIndex, 5, 1, 0);
                        this->poke_default(WorkingData_25, working4thAmp, 4, 0, 0);
                        this->poke_default(WorkingData_25, working4thIndex, 4, 1, 0);
                        this->poke_default(WorkingData_25, working3rdAmp, 3, 0, 0);
                        this->poke_default(WorkingData_25, working3rdIndex, 3, 1, 0);
                        this->poke_default(WorkingData_25, working2ndAmp, 2, 0, 0);
                        this->poke_default(WorkingData_25, working2ndIndex, 2, 1, 0);
                        this->poke_default(WorkingData_25, workingMaxAmp, 1, 0, 0);
                        this->poke_default(WorkingData_25, workingMaxIndex, 1, 1, 0);
                        this->poke_default(WorkingData_25, lastAmp_24, 0, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 0, 1, 0);
                    } else if (lastAmp_24 > working2ndAmp) {
                        this->poke_default(WorkingData_25, working5thAmp, 5, 0, 0);
                        this->poke_default(WorkingData_25, working5thIndex, 5, 1, 0);
                        this->poke_default(WorkingData_25, working4thAmp, 4, 0, 0);
                        this->poke_default(WorkingData_25, working4thIndex, 4, 1, 0);
                        this->poke_default(WorkingData_25, working3rdAmp, 3, 0, 0);
                        this->poke_default(WorkingData_25, working3rdIndex, 3, 1, 0);
                        this->poke_default(WorkingData_25, working2ndAmp, 2, 0, 0);
                        this->poke_default(WorkingData_25, working2ndIndex, 2, 1, 0);
                        this->poke_default(WorkingData_25, lastAmp_24, 1, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 1, 1, 0);
                    } else if (lastAmp_24 > working3rdAmp) {
                        this->poke_default(WorkingData_25, working5thAmp, 5, 0, 0);
                        this->poke_default(WorkingData_25, working5thIndex, 5, 1, 0);
                        this->poke_default(WorkingData_25, working4thAmp, 4, 0, 0);
                        this->poke_default(WorkingData_25, working4thIndex, 4, 1, 0);
                        this->poke_default(WorkingData_25, working3rdAmp, 3, 0, 0);
                        this->poke_default(WorkingData_25, working3rdIndex, 3, 1, 0);
                        this->poke_default(WorkingData_25, lastAmp_24, 2, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 2, 1, 0);
                    } else if (lastAmp_24 > working4thAmp) {
                        this->poke_default(WorkingData_25, working5thAmp, 5, 0, 0);
                        this->poke_default(WorkingData_25, working5thIndex, 5, 1, 0);
                        this->poke_default(WorkingData_25, working4thAmp, 4, 0, 0);
                        this->poke_default(WorkingData_25, working4thIndex, 4, 1, 0);
                        this->poke_default(WorkingData_25, lastAmp_24, 3, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 3, 1, 0);
                    } else if (lastAmp_24 > working5thAmp) {
                        this->poke_default(WorkingData_25, working5thAmp, 5, 0, 0);
                        this->poke_default(WorkingData_25, working5thIndex, 5, 1, 0);
                        this->poke_default(WorkingData_25, lastAmp_24, 4, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 4, 1, 0);
                    } else if (lastAmp_24 > working6thAmp) {
                        this->poke_default(WorkingData_25, lastAmp_24, 5, 0, 0);
                        this->poke_default(WorkingData_25, k_23, 5, 1, 0);
                    }
                }

                this->poke_default(this->gen_01_sumCosBuff, 0, k, 0, 0);
                this->poke_default(this->gen_01_sumSinBuff, 0, k, 0, 0);
                this->poke_default(this->gen_01_countBuff, 0, k, 0, 0);
                this->poke_default(this->gen_01_phaseBuff, 0, k, 0, 0);
            } else {
                this->poke_default(this->gen_01_countBuff, count1A + 1, k, 0, 0);
                this->poke_default(this->gen_01_phaseBuff, phase1A + phaseStep_5, k, 0, 0);
            }

            if (count1B > winLength_4 + winLength_4 / (number)2) {
                number cartopol_3 = 0;
                number cartopol_4 = 0;
                cartopol_3 = (sumCos1B * sumCos1B + sumSin1B * sumSin1B <= 0 ? 0 : rnbo_sqrt(sumCos1B * sumCos1B + sumSin1B * sumSin1B)), cartopol_4 = rnbo_atan2(sumSin1B, sumCos1B);
                number lastAmpB_38 = ((winLength_4 == 0. ? 0. : cartopol_3 / winLength_4)) * 4;

                if (lastAmpB_38 < noiseFloor * 0.01) {
                    lastAmpB_38 = 0;
                }

                this->poke_default(this->gen_01_outputBuff, lastAmpB_38, k, 0, 0);

                if (k == 0) {
                    {
                        auto WorkingData_40 = this->gen_01_workingData;
                        auto CurrentData_39 = this->gen_01_currentData;

                        for (number i = 0; i < this->dim(WorkingData_40); i = i + 1) {
                            number currentMaxAmp = 0;
                            number _ = 0;
                            auto result_41 = this->peek_default(WorkingData_40, i, 0);
                            _ = result_41[1];
                            currentMaxAmp = result_41[0];
                            this->poke_default(CurrentData_39, currentMaxAmp, i, 0, 0);
                            number currentMaxIndex = 0;
                            auto result_42 = this->peek_default(WorkingData_40, i, 1);
                            _ = result_42[1];
                            currentMaxIndex = result_42[0];
                            this->poke_default(CurrentData_39, currentMaxIndex, i, 1, 0);
                            this->poke_default(WorkingData_40, 0, i, 0, 0);
                            this->poke_default(WorkingData_40, 0, i, 1, 0);
                        }
                    }
                }

                this->poke_default(this->gen_01_sumCosBuff, 0, k, 1, 0);
                this->poke_default(this->gen_01_sumSinBuff, 0, k, 1, 0);
                this->poke_default(this->gen_01_countBuff, winLength_4 / (number)2, k, 1, 0);
                this->poke_default(this->gen_01_phaseBuff, 0, k, 1, 0);
            } else {
                this->poke_default(this->gen_01_countBuff, count1B + 1, k, 1, 0);
                this->poke_default(this->gen_01_phaseBuff, phase1B + phaseStep_5, k, 1, 0);
            }
        }

        for (number i = 0; i <= 1; i = i + 1) {
            {
                auto overtone_48 = 12;
                auto num_47 = i;
                auto OutputBuff_46 = this->gen_01_outputBuff;
                auto PolyTolerance_45 = polyTolerance;
                auto OvertoneThreshold_44 = overtoneThreshold;
                auto CurrentMaxData_43 = this->gen_01_currentData;
                number CurrentMaxAmp = 0;
                number _ = 0;
                auto result_49 = this->peek_default(CurrentMaxData_43, num_47, 0);
                _ = result_49[1];
                CurrentMaxAmp = result_49[0];
                number CurrentMaxIndex = 0;
                auto result_50 = this->peek_default(CurrentMaxData_43, num_47, 1);
                _ = result_50[1];
                CurrentMaxIndex = result_50[0];
                number harmonicAmp = 0;
                number harmonicIndex = 0;
                auto result_51 = this->peek_boundmode_ignore(OutputBuff_46, CurrentMaxIndex + overtone_48, 0);
                harmonicIndex = result_51[1];
                harmonicAmp = result_51[0];

                if (CurrentMaxAmp * (OvertoneThreshold_44 * 0.01) < harmonicAmp && ((harmonicAmp == 0. ? 0. : rnbo_abs(CurrentMaxAmp - harmonicAmp) / harmonicAmp)) > PolyTolerance_45 * 0.01) {
                    this->poke_default(OutputBuff_46, harmonicAmp * 0, harmonicIndex, 0, 0);
                }
            }

            {
                auto overtone_57 = 19;
                auto num_56 = i;
                auto OutputBuff_55 = this->gen_01_outputBuff;
                auto PolyTolerance_54 = polyTolerance;
                auto OvertoneThreshold_53 = overtoneThreshold;
                auto CurrentMaxData_52 = this->gen_01_currentData;
                number CurrentMaxAmp = 0;
                number _ = 0;
                auto result_58 = this->peek_default(CurrentMaxData_52, num_56, 0);
                _ = result_58[1];
                CurrentMaxAmp = result_58[0];
                number CurrentMaxIndex = 0;
                auto result_59 = this->peek_default(CurrentMaxData_52, num_56, 1);
                _ = result_59[1];
                CurrentMaxIndex = result_59[0];
                number harmonicAmp = 0;
                number harmonicIndex = 0;
                auto result_60 = this->peek_boundmode_ignore(OutputBuff_55, CurrentMaxIndex + overtone_57, 0);
                harmonicIndex = result_60[1];
                harmonicAmp = result_60[0];

                if (CurrentMaxAmp * (OvertoneThreshold_53 * 0.01) < harmonicAmp && ((harmonicAmp == 0. ? 0. : rnbo_abs(CurrentMaxAmp - harmonicAmp) / harmonicAmp)) > PolyTolerance_54 * 0.01) {
                    this->poke_default(OutputBuff_55, harmonicAmp * 0, harmonicIndex, 0, 0);
                }
            }

            {
                auto overtone_66 = 24;
                auto num_65 = i;
                auto OutputBuff_64 = this->gen_01_outputBuff;
                auto PolyTolerance_63 = polyTolerance;
                auto OvertoneThreshold_62 = overtoneThreshold;
                auto CurrentMaxData_61 = this->gen_01_currentData;
                number CurrentMaxAmp = 0;
                number _ = 0;
                auto result_67 = this->peek_default(CurrentMaxData_61, num_65, 0);
                _ = result_67[1];
                CurrentMaxAmp = result_67[0];
                number CurrentMaxIndex = 0;
                auto result_68 = this->peek_default(CurrentMaxData_61, num_65, 1);
                _ = result_68[1];
                CurrentMaxIndex = result_68[0];
                number harmonicAmp = 0;
                number harmonicIndex = 0;
                auto result_69 = this->peek_boundmode_ignore(OutputBuff_64, CurrentMaxIndex + overtone_66, 0);
                harmonicIndex = result_69[1];
                harmonicAmp = result_69[0];

                if (CurrentMaxAmp * (OvertoneThreshold_62 * 0.01) < harmonicAmp && ((harmonicAmp == 0. ? 0. : rnbo_abs(CurrentMaxAmp - harmonicAmp) / harmonicAmp)) > PolyTolerance_63 * 0.01) {
                    this->poke_default(OutputBuff_64, harmonicAmp * 0, harmonicIndex, 0, 0);
                }
            }

            {
                auto overtone_75 = 28;
                auto num_74 = i;
                auto OutputBuff_73 = this->gen_01_outputBuff;
                auto PolyTolerance_72 = polyTolerance;
                auto OvertoneThreshold_71 = overtoneThreshold;
                auto CurrentMaxData_70 = this->gen_01_currentData;
                number CurrentMaxAmp = 0;
                number _ = 0;
                auto result_76 = this->peek_default(CurrentMaxData_70, num_74, 0);
                _ = result_76[1];
                CurrentMaxAmp = result_76[0];
                number CurrentMaxIndex = 0;
                auto result_77 = this->peek_default(CurrentMaxData_70, num_74, 1);
                _ = result_77[1];
                CurrentMaxIndex = result_77[0];
                number harmonicAmp = 0;
                number harmonicIndex = 0;
                auto result_78 = this->peek_boundmode_ignore(OutputBuff_73, CurrentMaxIndex + overtone_75, 0);
                harmonicIndex = result_78[1];
                harmonicAmp = result_78[0];

                if (CurrentMaxAmp * (OvertoneThreshold_71 * 0.01) < harmonicAmp && ((harmonicAmp == 0. ? 0. : rnbo_abs(CurrentMaxAmp - harmonicAmp) / harmonicAmp)) > PolyTolerance_72 * 0.01) {
                    this->poke_default(OutputBuff_73, harmonicAmp * 0, harmonicIndex, 0, 0);
                }
            }
        }

        number expr_5_79 = in1[(Index)i0];
        number expr_6_80 = in2[(Index)i0];
        number currentMaxIndex1 = 0;
        number _ = 0;
        auto result_81 = this->peek_default(this->gen_01_currentData, 0, 1);
        _ = result_81[1];
        currentMaxIndex1 = result_81[0];
        number currentMaxIndex2 = 0;
        auto result_82 = this->peek_default(this->gen_01_currentData, 1, 1);
        _ = result_82[1];
        currentMaxIndex2 = result_82[0];
        number expr_7_83 = currentMaxIndex1 + 21;
        number expr_8_84 = currentMaxIndex2 + 21;
        out1[(Index)i0] = expr_5_79;
        out4[(Index)i0] = expr_8_84;
        out2[(Index)i0] = expr_6_80;
        out3[(Index)i0] = expr_7_83;
    }
}

void numbertilde_01_perform(const SampleValue * input_signal, SampleValue * output, Index n) {
    auto __numbertilde_01_currentIntervalInSamples = this->numbertilde_01_currentIntervalInSamples;
    auto __numbertilde_01_lastValue = this->numbertilde_01_lastValue;
    auto __numbertilde_01_currentInterval = this->numbertilde_01_currentInterval;
    auto __numbertilde_01_rampInSamples = this->numbertilde_01_rampInSamples;
    auto __numbertilde_01_outValue = this->numbertilde_01_outValue;
    auto __numbertilde_01_currentMode = this->numbertilde_01_currentMode;
    number monitorvalue = input_signal[0];

    for (Index i = 0; i < n; i++) {
        if (__numbertilde_01_currentMode == 0) {
            output[(Index)i] = this->numbertilde_01_smooth_next(
                __numbertilde_01_outValue,
                __numbertilde_01_rampInSamples,
                __numbertilde_01_rampInSamples
            );
        } else {
            output[(Index)i] = input_signal[(Index)i];
        }
    }

    __numbertilde_01_currentInterval -= n;

    if (monitorvalue != __numbertilde_01_lastValue && __numbertilde_01_currentInterval <= 0) {
        __numbertilde_01_currentInterval = __numbertilde_01_currentIntervalInSamples;

        this->getEngine()->scheduleClockEventWithValue(
            this,
            2098551528,
            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
            monitorvalue
        );;

        __numbertilde_01_lastValue = monitorvalue;
        ;
    }

    this->numbertilde_01_currentInterval = __numbertilde_01_currentInterval;
    this->numbertilde_01_lastValue = __numbertilde_01_lastValue;
}

void numbertilde_02_perform(const SampleValue * input_signal, SampleValue * output, Index n) {
    auto __numbertilde_02_currentIntervalInSamples = this->numbertilde_02_currentIntervalInSamples;
    auto __numbertilde_02_lastValue = this->numbertilde_02_lastValue;
    auto __numbertilde_02_currentInterval = this->numbertilde_02_currentInterval;
    auto __numbertilde_02_rampInSamples = this->numbertilde_02_rampInSamples;
    auto __numbertilde_02_outValue = this->numbertilde_02_outValue;
    auto __numbertilde_02_currentMode = this->numbertilde_02_currentMode;
    number monitorvalue = input_signal[0];

    for (Index i = 0; i < n; i++) {
        if (__numbertilde_02_currentMode == 0) {
            output[(Index)i] = this->numbertilde_02_smooth_next(
                __numbertilde_02_outValue,
                __numbertilde_02_rampInSamples,
                __numbertilde_02_rampInSamples
            );
        } else {
            output[(Index)i] = input_signal[(Index)i];
        }
    }

    __numbertilde_02_currentInterval -= n;

    if (monitorvalue != __numbertilde_02_lastValue && __numbertilde_02_currentInterval <= 0) {
        __numbertilde_02_currentInterval = __numbertilde_02_currentIntervalInSamples;

        this->getEngine()->scheduleClockEventWithValue(
            this,
            -1494586265,
            this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
            monitorvalue
        );;

        __numbertilde_02_lastValue = monitorvalue;
        ;
    }

    this->numbertilde_02_currentInterval = __numbertilde_02_currentInterval;
    this->numbertilde_02_lastValue = __numbertilde_02_lastValue;
}

void stackprotect_perform(Index n) {
    RNBO_UNUSED(n);
    auto __stackprotect_count = this->stackprotect_count;
    __stackprotect_count = 0;
    this->stackprotect_count = __stackprotect_count;
}

void data_01_srout_set(number ) {}

void data_01_chanout_set(number ) {}

void data_01_sizeout_set(number v) {
    this->data_01_sizeout = v;
}

void param_01_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_01_value;
}

void param_01_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_01_value_set(preset["value"]);
}

void param_02_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_02_value;
}

void param_02_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_02_value_set(preset["value"]);
}

void param_03_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_03_value;
}

void param_03_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_03_value_set(preset["value"]);
}

number gen_01_mtof_2_next(number midivalue, number tuning) {
    RNBO_UNUSED(tuning);

    if (midivalue == this->gen_01_mtof_2_lastInValue && 440 == this->gen_01_mtof_2_lastTuning)
        return this->gen_01_mtof_2_lastOutValue;

    this->gen_01_mtof_2_lastInValue = midivalue;
    this->gen_01_mtof_2_lastTuning = 440;
    number result = 0;

    {
        result = rnbo_exp(.057762265 * (midivalue - 69.0));
    }

    this->gen_01_mtof_2_lastOutValue = 440 * result;
    return this->gen_01_mtof_2_lastOutValue;
}

void gen_01_mtof_2_reset() {
    this->gen_01_mtof_2_lastInValue = 0;
    this->gen_01_mtof_2_lastOutValue = 0;
    this->gen_01_mtof_2_lastTuning = 0;
}

void param_04_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_04_value;
}

void param_04_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_04_value_set(preset["value"]);
}

number numbertilde_01_smooth_d_next(number x) {
    number temp = (number)(x - this->numbertilde_01_smooth_d_prev);
    this->numbertilde_01_smooth_d_prev = x;
    return temp;
}

void numbertilde_01_smooth_d_dspsetup() {
    this->numbertilde_01_smooth_d_reset();
}

void numbertilde_01_smooth_d_reset() {
    this->numbertilde_01_smooth_d_prev = 0;
}

number numbertilde_01_smooth_next(number x, number up, number down) {
    if (this->numbertilde_01_smooth_d_next(x) != 0.) {
        if (x > this->numbertilde_01_smooth_prev) {
            number _up = up;

            if (_up < 1)
                _up = 1;

            this->numbertilde_01_smooth_index = _up;
            this->numbertilde_01_smooth_increment = (x - this->numbertilde_01_smooth_prev) / _up;
        } else if (x < this->numbertilde_01_smooth_prev) {
            number _down = down;

            if (_down < 1)
                _down = 1;

            this->numbertilde_01_smooth_index = _down;
            this->numbertilde_01_smooth_increment = (x - this->numbertilde_01_smooth_prev) / _down;
        }
    }

    if (this->numbertilde_01_smooth_index > 0) {
        this->numbertilde_01_smooth_prev += this->numbertilde_01_smooth_increment;
        this->numbertilde_01_smooth_index -= 1;
    } else {
        this->numbertilde_01_smooth_prev = x;
    }

    return this->numbertilde_01_smooth_prev;
}

void numbertilde_01_smooth_reset() {
    this->numbertilde_01_smooth_prev = 0;
    this->numbertilde_01_smooth_index = 0;
    this->numbertilde_01_smooth_increment = 0;
    this->numbertilde_01_smooth_d_reset();
}

void numbertilde_01_init() {
    this->numbertilde_01_currentMode = 1;
}

void numbertilde_01_dspsetup(bool force) {
    if ((bool)(this->numbertilde_01_setupDone) && (bool)(!(bool)(force)))
        return;

    this->numbertilde_01_currentIntervalInSamples = this->mstosamps(100);
    this->numbertilde_01_currentInterval = this->numbertilde_01_currentIntervalInSamples;
    this->numbertilde_01_rampInSamples = this->mstosamps(this->numbertilde_01_ramp);
    this->numbertilde_01_setupDone = true;
    this->numbertilde_01_smooth_d_dspsetup();
}

number numbertilde_02_smooth_d_next(number x) {
    number temp = (number)(x - this->numbertilde_02_smooth_d_prev);
    this->numbertilde_02_smooth_d_prev = x;
    return temp;
}

void numbertilde_02_smooth_d_dspsetup() {
    this->numbertilde_02_smooth_d_reset();
}

void numbertilde_02_smooth_d_reset() {
    this->numbertilde_02_smooth_d_prev = 0;
}

number numbertilde_02_smooth_next(number x, number up, number down) {
    if (this->numbertilde_02_smooth_d_next(x) != 0.) {
        if (x > this->numbertilde_02_smooth_prev) {
            number _up = up;

            if (_up < 1)
                _up = 1;

            this->numbertilde_02_smooth_index = _up;
            this->numbertilde_02_smooth_increment = (x - this->numbertilde_02_smooth_prev) / _up;
        } else if (x < this->numbertilde_02_smooth_prev) {
            number _down = down;

            if (_down < 1)
                _down = 1;

            this->numbertilde_02_smooth_index = _down;
            this->numbertilde_02_smooth_increment = (x - this->numbertilde_02_smooth_prev) / _down;
        }
    }

    if (this->numbertilde_02_smooth_index > 0) {
        this->numbertilde_02_smooth_prev += this->numbertilde_02_smooth_increment;
        this->numbertilde_02_smooth_index -= 1;
    } else {
        this->numbertilde_02_smooth_prev = x;
    }

    return this->numbertilde_02_smooth_prev;
}

void numbertilde_02_smooth_reset() {
    this->numbertilde_02_smooth_prev = 0;
    this->numbertilde_02_smooth_index = 0;
    this->numbertilde_02_smooth_increment = 0;
    this->numbertilde_02_smooth_d_reset();
}

void numbertilde_02_init() {
    this->numbertilde_02_currentMode = 1;
}

void numbertilde_02_dspsetup(bool force) {
    if ((bool)(this->numbertilde_02_setupDone) && (bool)(!(bool)(force)))
        return;

    this->numbertilde_02_currentIntervalInSamples = this->mstosamps(100);
    this->numbertilde_02_currentInterval = this->numbertilde_02_currentIntervalInSamples;
    this->numbertilde_02_rampInSamples = this->mstosamps(this->numbertilde_02_ramp);
    this->numbertilde_02_setupDone = true;
    this->numbertilde_02_smooth_d_dspsetup();
}

void data_01_init() {
    this->data_01_buffer->setWantsFill(true);
}

Index data_01_evaluateSizeExpr(number samplerate, number vectorsize) {
    RNBO_UNUSED(vectorsize);
    RNBO_UNUSED(samplerate);
    number size = 0;
    return (Index)(size);
}

void data_01_dspsetup(bool force) {
    if ((bool)(this->data_01_setupDone) && (bool)(!(bool)(force)))
        return;

    if (this->data_01_sizemode == 2) {
        this->data_01_buffer = this->data_01_buffer->setSize((Index)(this->mstosamps(this->data_01_sizems)));
        updateDataRef(this, this->data_01_buffer);
    } else if (this->data_01_sizemode == 3) {
        this->data_01_buffer = this->data_01_buffer->setSize(this->data_01_evaluateSizeExpr(this->sr, this->vs));
        updateDataRef(this, this->data_01_buffer);
    }

    this->data_01_setupDone = true;
}

void data_01_bufferUpdated() {
    this->data_01_report();
}

void data_01_report() {
    this->data_01_srout_set(this->data_01_buffer->getSampleRate());
    this->data_01_chanout_set(this->data_01_buffer->getChannels());
    this->data_01_sizeout_set(this->data_01_buffer->getSize());
}

void globaltransport_advance() {}

void globaltransport_dspsetup(bool ) {}

bool stackprotect_check() {
    this->stackprotect_count++;

    if (this->stackprotect_count > 128) {
        console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
        return true;
    }

    return false;
}

Index getPatcherSerial() const {
    return 0;
}

void sendParameter(ParameterIndex index, bool ignoreValue) {
    this->getEngine()->notifyParameterValueChanged(index, (ignoreValue ? 0 : this->getParameterValue(index)), ignoreValue);
}

void scheduleParamInit(ParameterIndex index, Index order) {
    this->paramInitIndices->push(index);
    this->paramInitOrder->push(order);
}

void processParamInitEvents() {
    this->listquicksort(
        this->paramInitOrder,
        this->paramInitIndices,
        0,
        (int)(this->paramInitOrder->length - 1),
        true
    );

    for (Index i = 0; i < this->paramInitOrder->length; i++) {
        this->getEngine()->scheduleParameterBang(this->paramInitIndices[i], 0);
    }
}

void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
    RNBO_UNUSED(inProcess);
    RNBO_UNUSED(engine);
    this->_currentTime = time;
    auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));

    if (offset >= (SampleIndex)(this->vs))
        offset = (SampleIndex)(this->vs) - 1;

    if (offset < 0)
        offset = 0;

    this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
}

void assign_defaults()
{
    param_01_value = 1.5;
    param_02_value = 20;
    param_03_value = 50;
    gen_01_in1 = 0;
    gen_01_in2 = 0;
    gen_01_Q_factor = 0;
    gen_01_overtoneThreshold = 0;
    gen_01_polyTolerance = 0;
    gen_01_noiseFloor = 0;
    param_04_value = 5;
    numbertilde_01_input_number = 0;
    numbertilde_01_ramp = 0;
    numbertilde_02_input_number = 0;
    numbertilde_02_ramp = 0;
    data_01_sizeout = 0;
    data_01_size = 88;
    data_01_sizems = 0;
    data_01_normalize = 0.995;
    data_01_channels = 1;
    ctlin_01_input = 0;
    ctlin_01_controller = 0;
    ctlin_01_channel = -1;
    expr_01_in1 = 0;
    expr_01_in2 = 0.007874015748;
    expr_01_out1 = 0;
    ctlin_02_input = 0;
    ctlin_02_controller = 0;
    ctlin_02_channel = -1;
    expr_02_in1 = 0;
    expr_02_in2 = 0.007874015748;
    expr_02_out1 = 0;
    ctlin_03_input = 0;
    ctlin_03_controller = 0;
    ctlin_03_channel = -1;
    expr_03_in1 = 0;
    expr_03_in2 = 0.007874015748;
    expr_03_out1 = 0;
    ctlin_04_input = 0;
    ctlin_04_controller = 0;
    ctlin_04_channel = -1;
    expr_04_in1 = 0;
    expr_04_in2 = 0.007874015748;
    expr_04_out1 = 0;
    _currentTime = 0;
    audioProcessSampleCount = 0;
    sampleOffsetIntoNextAudioBuffer = 0;
    zeroBuffer = nullptr;
    dummyBuffer = nullptr;
    signals[0] = nullptr;
    signals[1] = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    param_01_lastValue = 0;
    param_02_lastValue = 0;
    param_03_lastValue = 0;
    gen_01_mtof_2_lastInValue = 0;
    gen_01_mtof_2_lastOutValue = 0;
    gen_01_mtof_2_lastTuning = 0;
    param_04_lastValue = 0;
    numbertilde_01_currentInterval = 0;
    numbertilde_01_currentIntervalInSamples = 0;
    numbertilde_01_lastValue = 0;
    numbertilde_01_outValue = 0;
    numbertilde_01_rampInSamples = 0;
    numbertilde_01_currentMode = 0;
    numbertilde_01_smooth_d_prev = 0;
    numbertilde_01_smooth_prev = 0;
    numbertilde_01_smooth_index = 0;
    numbertilde_01_smooth_increment = 0;
    numbertilde_01_setupDone = false;
    numbertilde_02_currentInterval = 0;
    numbertilde_02_currentIntervalInSamples = 0;
    numbertilde_02_lastValue = 0;
    numbertilde_02_outValue = 0;
    numbertilde_02_rampInSamples = 0;
    numbertilde_02_currentMode = 0;
    numbertilde_02_smooth_d_prev = 0;
    numbertilde_02_smooth_prev = 0;
    numbertilde_02_smooth_index = 0;
    numbertilde_02_smooth_increment = 0;
    numbertilde_02_setupDone = false;
    data_01_sizemode = 1;
    data_01_setupDone = false;
    ctlin_01_status = 0;
    ctlin_01_byte1 = -1;
    ctlin_01_inchan = 0;
    ctlin_02_status = 0;
    ctlin_02_byte1 = -1;
    ctlin_02_inchan = 0;
    ctlin_03_status = 0;
    ctlin_03_byte1 = -1;
    ctlin_03_inchan = 0;
    ctlin_04_status = 0;
    ctlin_04_byte1 = -1;
    ctlin_04_inchan = 0;
    globaltransport_tempo = nullptr;
    globaltransport_state = nullptr;
    stackprotect_count = 0;
    _voiceIndex = 0;
    _noteNumber = 0;
    isMuted = 1;
}

    // data ref strings
    struct DataRefStrings {
    	static constexpr auto& name0 = "OutputBuff";
    	static constexpr auto& file0 = "";
    	static constexpr auto& tag0 = "buffer~";
    	static constexpr auto& name1 = "countBuff";
    	static constexpr auto& file1 = "";
    	static constexpr auto& tag1 = "buffer~";
    	static constexpr auto& name2 = "phaseBuff";
    	static constexpr auto& file2 = "";
    	static constexpr auto& tag2 = "buffer~";
    	static constexpr auto& name3 = "sumCosBuff";
    	static constexpr auto& file3 = "";
    	static constexpr auto& tag3 = "buffer~";
    	static constexpr auto& name4 = "sumSinBuff";
    	static constexpr auto& file4 = "";
    	static constexpr auto& tag4 = "buffer~";
    	static constexpr auto& name5 = "currentData";
    	static constexpr auto& file5 = "";
    	static constexpr auto& tag5 = "buffer~";
    	static constexpr auto& name6 = "workingData";
    	static constexpr auto& file6 = "";
    	static constexpr auto& tag6 = "buffer~";
    	static constexpr auto& name7 = "RNBODefaultMtofLookupTable256";
    	static constexpr auto& file7 = "";
    	static constexpr auto& tag7 = "buffer~";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    number param_01_value;
    number param_02_value;
    number param_03_value;
    number gen_01_in1;
    number gen_01_in2;
    number gen_01_Q_factor;
    number gen_01_overtoneThreshold;
    number gen_01_polyTolerance;
    number gen_01_noiseFloor;
    number param_04_value;
    number numbertilde_01_input_number;
    number numbertilde_01_ramp;
    number numbertilde_02_input_number;
    number numbertilde_02_ramp;
    number data_01_sizeout;
    number data_01_size;
    number data_01_sizems;
    number data_01_normalize;
    number data_01_channels;
    number ctlin_01_input;
    number ctlin_01_controller;
    number ctlin_01_channel;
    number expr_01_in1;
    number expr_01_in2;
    number expr_01_out1;
    number ctlin_02_input;
    number ctlin_02_controller;
    number ctlin_02_channel;
    number expr_02_in1;
    number expr_02_in2;
    number expr_02_out1;
    number ctlin_03_input;
    number ctlin_03_controller;
    number ctlin_03_channel;
    number expr_03_in1;
    number expr_03_in2;
    number expr_03_out1;
    number ctlin_04_input;
    number ctlin_04_controller;
    number ctlin_04_channel;
    number expr_04_in1;
    number expr_04_in2;
    number expr_04_out1;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    SampleValue * signals[2];
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    number param_01_lastValue;
    number param_02_lastValue;
    number param_03_lastValue;
    Float32BufferRef gen_01_outputBuff;
    SampleBufferRef gen_01_countBuff;
    SampleBufferRef gen_01_phaseBuff;
    SampleBufferRef gen_01_sumCosBuff;
    SampleBufferRef gen_01_sumSinBuff;
    SampleBufferRef gen_01_currentData;
    SampleBufferRef gen_01_workingData;
    number gen_01_mtof_2_lastInValue;
    number gen_01_mtof_2_lastOutValue;
    number gen_01_mtof_2_lastTuning;
    SampleBufferRef gen_01_mtof_2_buffer;
    number param_04_lastValue;
    SampleIndex numbertilde_01_currentInterval;
    SampleIndex numbertilde_01_currentIntervalInSamples;
    number numbertilde_01_lastValue;
    number numbertilde_01_outValue;
    number numbertilde_01_rampInSamples;
    Int numbertilde_01_currentMode;
    number numbertilde_01_smooth_d_prev;
    number numbertilde_01_smooth_prev;
    number numbertilde_01_smooth_index;
    number numbertilde_01_smooth_increment;
    bool numbertilde_01_setupDone;
    SampleIndex numbertilde_02_currentInterval;
    SampleIndex numbertilde_02_currentIntervalInSamples;
    number numbertilde_02_lastValue;
    number numbertilde_02_outValue;
    number numbertilde_02_rampInSamples;
    Int numbertilde_02_currentMode;
    number numbertilde_02_smooth_d_prev;
    number numbertilde_02_smooth_prev;
    number numbertilde_02_smooth_index;
    number numbertilde_02_smooth_increment;
    bool numbertilde_02_setupDone;
    Float32BufferRef data_01_buffer;
    Int data_01_sizemode;
    bool data_01_setupDone;
    Int ctlin_01_status;
    Int ctlin_01_byte1;
    Int ctlin_01_inchan;
    Int ctlin_02_status;
    Int ctlin_02_byte1;
    Int ctlin_02_inchan;
    Int ctlin_03_status;
    Int ctlin_03_byte1;
    Int ctlin_03_inchan;
    Int ctlin_04_status;
    Int ctlin_04_byte1;
    Int ctlin_04_inchan;
    signal globaltransport_tempo;
    signal globaltransport_state;
    number stackprotect_count;
    DataRef OutputBuff;
    DataRef countBuff;
    DataRef phaseBuff;
    DataRef sumCosBuff;
    DataRef sumSinBuff;
    DataRef currentData;
    DataRef workingData;
    DataRef RNBODefaultMtofLookupTable256;
    Index _voiceIndex;
    Int _noteNumber;
    Index isMuted;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
    bool _isInitialized = false;
};

static PatcherInterface* creaternbomatic()
{
    return new rnbomatic<EXTERNALENGINE>();
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" PatcherFactoryFunctionPtr GetPatcherFactoryFunction()
#else
extern "C" PatcherFactoryFunctionPtr rnbomaticFactoryFunction()
#endif
{
    return creaternbomatic;
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" void SetLogger(Logger* logger)
#else
void rnbomaticSetLogger(Logger* logger)
#endif
{
    console = logger;
}

} // end RNBO namespace

