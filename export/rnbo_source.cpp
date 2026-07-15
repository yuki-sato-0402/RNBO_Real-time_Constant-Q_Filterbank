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
    this->gen_01_perform(in1, in2, this->gen_01_Q_factor, out1, out2, n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
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
    return 6;
}

void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (index == 0) {
        this->data_01_buffer = reInitDataView(this->data_01_buffer, this->OutputBuff);
        this->data_01_bufferUpdated();
        this->gen_01_outputBuff = reInitDataView(this->gen_01_outputBuff, this->OutputBuff);
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

    this->RNBODefaultMtofLookupTable256 = initDataRef(
        this->RNBODefaultMtofLookupTable256,
        this->dataRefStrings->name5,
        true,
        this->dataRefStrings->file5,
        this->dataRefStrings->tag5
    );

    this->assign_defaults();
    this->applyState();
    this->OutputBuff->setIndex(0);
    this->data_01_buffer = new Float32Buffer(this->OutputBuff);
    this->gen_01_outputBuff = new Float32Buffer(this->OutputBuff);
    this->countBuff->setIndex(1);
    this->gen_01_countBuff = new SampleBuffer(this->countBuff);
    this->phaseBuff->setIndex(2);
    this->gen_01_phaseBuff = new SampleBuffer(this->phaseBuff);
    this->sumCosBuff->setIndex(3);
    this->gen_01_sumCosBuff = new SampleBuffer(this->sumCosBuff);
    this->sumSinBuff->setIndex(4);
    this->gen_01_sumSinBuff = new SampleBuffer(this->sumSinBuff);
    this->RNBODefaultMtofLookupTable256->setIndex(5);
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
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->param_01_setPresetValue(getSubState(preset, "Q_factor"));
}

void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case 0:
        {
        this->param_01_value_set(v);
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
    return 1;
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "Q_factor";
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
            info->initialValue = 1;
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
    case 0:
        {
        {
            value = (value < 1 ? 1 : (value > 2 ? 2 : value));
            ParameterValue normalizedValue = (value - 1) / (2 - 1);
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
    case 0:
        {
        {
            {
                return 1 + value * (2 - 1);
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
    default:
        {
        return value;
        }
    }
}

void processNumMessage(MessageTag , MessageTag , MillisecondTime , number ) {}

void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}

void processBangMessage(MessageTag , MessageTag , MillisecondTime ) {}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {

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

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
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

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

void deallocateSignals() {
    Index i;
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
    case 5:
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
    this->gen_01_mtof_2_buffer->requestSize(65536, 1);
    this->gen_01_mtof_2_buffer->setSampleRate(this->sr);
    this->data_01_buffer = this->data_01_buffer->allocateIfNeeded();
    this->gen_01_outputBuff = this->gen_01_outputBuff->allocateIfNeeded();

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

    this->gen_01_mtof_2_buffer = this->gen_01_mtof_2_buffer->allocateIfNeeded();

    if (this->RNBODefaultMtofLookupTable256->hasRequestedSize()) {
        if (this->RNBODefaultMtofLookupTable256->wantsFill())
            this->fillRNBODefaultMtofLookupTable256(this->RNBODefaultMtofLookupTable256);

        this->getEngine()->sendDataRefUpdated(5);
    }
}

void initializeObjects() {
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

void processClockEvent(MillisecondTime , ClockId , bool , ParameterValue ) {}

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

    this->processParamInitEvents();
}

number param_01_value_constrain(number v) const {
    v = (v > 2 ? 2 : (v < 1 ? 1 : v));
    return v;
}

void gen_01_Q_factor_set(number v) {
    this->gen_01_Q_factor = v;
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

void gen_01_perform(
    const Sample * in1,
    const Sample * in2,
    number Q_factor,
    SampleValue * out1,
    SampleValue * out2,
    Index n
) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        number Q_0 = 16.817153745106 * Q_factor;
        number sumInput_1 = (in1[(Index)i] + in2[(Index)i]) * 0.5;

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
                this->poke_default(this->gen_01_outputBuff, lastAmpA_18, k, 0, 0);
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
                number lastAmpB_19 = ((winLength_4 == 0. ? 0. : cartopol_3 / winLength_4)) * 4;
                this->poke_default(this->gen_01_outputBuff, lastAmpB_19, k, 0, 0);
                this->poke_default(this->gen_01_sumCosBuff, 0, k, 1, 0);
                this->poke_default(this->gen_01_sumSinBuff, 0, k, 1, 0);
                this->poke_default(this->gen_01_countBuff, winLength_4 / (number)2, k, 1, 0);
                this->poke_default(this->gen_01_phaseBuff, 0, k, 1, 0);
            } else {
                this->poke_default(this->gen_01_countBuff, count1B + 1, k, 1, 0);
                this->poke_default(this->gen_01_phaseBuff, phase1B + phaseStep_5, k, 1, 0);
            }
        }

        number expr_5_20 = in1[(Index)i];
        number expr_6_21 = in2[(Index)i];
        out2[(Index)i] = expr_6_21;
        out1[(Index)i] = expr_5_20;
    }
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
    param_01_value = 1;
    data_01_sizeout = 0;
    data_01_size = 88;
    data_01_sizems = 0;
    data_01_normalize = 0.995;
    data_01_channels = 1;
    gen_01_in1 = 0;
    gen_01_in2 = 0;
    gen_01_Q_factor = 0;
    ctlin_01_input = 0;
    ctlin_01_controller = 0;
    ctlin_01_channel = -1;
    expr_01_in1 = 0;
    expr_01_in2 = 0.007874015748;
    expr_01_out1 = 0;
    _currentTime = 0;
    audioProcessSampleCount = 0;
    sampleOffsetIntoNextAudioBuffer = 0;
    zeroBuffer = nullptr;
    dummyBuffer = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    param_01_lastValue = 0;
    data_01_sizemode = 1;
    data_01_setupDone = false;
    gen_01_mtof_2_lastInValue = 0;
    gen_01_mtof_2_lastOutValue = 0;
    gen_01_mtof_2_lastTuning = 0;
    ctlin_01_status = 0;
    ctlin_01_byte1 = -1;
    ctlin_01_inchan = 0;
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
    	static constexpr auto& name5 = "RNBODefaultMtofLookupTable256";
    	static constexpr auto& file5 = "";
    	static constexpr auto& tag5 = "buffer~";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    number param_01_value;
    number data_01_sizeout;
    number data_01_size;
    number data_01_sizems;
    number data_01_normalize;
    number data_01_channels;
    number gen_01_in1;
    number gen_01_in2;
    number gen_01_Q_factor;
    number ctlin_01_input;
    number ctlin_01_controller;
    number ctlin_01_channel;
    number expr_01_in1;
    number expr_01_in2;
    number expr_01_out1;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    number param_01_lastValue;
    Float32BufferRef data_01_buffer;
    Int data_01_sizemode;
    bool data_01_setupDone;
    Float32BufferRef gen_01_outputBuff;
    SampleBufferRef gen_01_countBuff;
    SampleBufferRef gen_01_phaseBuff;
    SampleBufferRef gen_01_sumCosBuff;
    SampleBufferRef gen_01_sumSinBuff;
    number gen_01_mtof_2_lastInValue;
    number gen_01_mtof_2_lastOutValue;
    number gen_01_mtof_2_lastTuning;
    SampleBufferRef gen_01_mtof_2_buffer;
    Int ctlin_01_status;
    Int ctlin_01_byte1;
    Int ctlin_01_inchan;
    signal globaltransport_tempo;
    signal globaltransport_state;
    number stackprotect_count;
    DataRef OutputBuff;
    DataRef countBuff;
    DataRef phaseBuff;
    DataRef sumCosBuff;
    DataRef sumSinBuff;
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

