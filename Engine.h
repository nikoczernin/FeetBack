#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace feetback {
constexpr double pi = 3.14159265358979323846;
inline float clean(float x, float fallback = 0) { return std::isfinite(x) ? x : fallback; }
inline float bounded(float x, float low, float high, float fallback) {
    return std::clamp(clean(x, fallback), low, high);
}
struct Parameters {
    float amount = 65, attackMs = 700, releaseMs = 600, sensitivity = 60;
    float toneHz = 6000, mix = 45;
    int harmonic = 0;
    bool engaged = true, bypass = false;
};

struct Biquad {
    double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    void lowpass(double sr, double hz) {
        const double w=2*pi*hz/sr, c=std::cos(w), alpha=std::sin(w)/std::sqrt(2.0);
        const double den=1+alpha;
        b0=(1-c)*0.5/den; b1=(1-c)/den; b2=b0; a1=-2*c/den; a2=(1-alpha)/den;
    }
    float tick(float input) {
        const double out=b0*input+z1;
        z1=b1*input-a1*out+z2; z2=b2*input-a2*out;
        return static_cast<float>(out);
    }
    void reset() { z1=z2=0; }
};

// Original implementation of the YIN cumulative-normalized difference method.
// de Cheveigne & Kawahara (2002), doi:10.1121/1.1458024.
// All storage is fixed; analysis runs at <= 12 kHz, independently of host buffers.
class Tracker {
public:
    void prepare(double sr) {
        factor=std::max(1,static_cast<int>(std::ceil(sr/12000.0)));
        rate=sr/factor; maximumLag=static_cast<int>(std::ceil(rate/55.0));
        minimumLag=std::max(2,static_cast<int>(std::floor(rate/1800.0)));
        hop=std::max(32,static_cast<int>(std::round(rate*0.008)));
        const auto cutoff=std::min(2800.0,rate*0.24);
        for(auto& f: antialias) f.lowpass(sr,cutoff);
        dcCoefficient=std::exp(-2*pi*35.0/sr);
        reset();
    }
    void reset() {
        ring.fill(0); frame.fill(0); difference.fill(1); rawDifference.fill(0);
        for(auto& f: antialias) f.reset();
        position=collected=decimator=hopCount=0;
        dcInput=dcOutput=0; valid=false; frequency=0; rms=confidence=0;
    }
    bool push(float input, float sensitivity) {
        const double high=input-dcInput+dcCoefficient*dcOutput;
        dcInput=input; dcOutput=high;
        float x=static_cast<float>(high);
        for(auto& f: antialias) x=f.tick(x);
        if (++decimator < factor) return false;
        decimator=0;
        ring[static_cast<size_t>(position)]=x;
        position=(position+1)%static_cast<int>(ring.size());
        collected=std::min(collected+1,static_cast<int>(ring.size()));
        if (++hopCount<hop) return false;
        hopCount=0;
        if(collected<window+maximumLag+2) return false;
        analyse(sensitivity);
        return true;
    }
    double interval() const { return hop/rate; }
    bool valid=false;
    float frequency=0,rms=0,confidence=0;
private:
    void analyse(float sensitivity) {
        const int count=window+maximumLag+2;
        const int start=(position-count+static_cast<int>(ring.size()))%static_cast<int>(ring.size());
        double energy=0;
        for(int i=0;i<count;++i) {
            frame[static_cast<size_t>(i)]=ring[static_cast<size_t>((start+i)%static_cast<int>(ring.size()))];
            if(i>=maximumLag) energy+=frame[static_cast<size_t>(i)]*frame[static_cast<size_t>(i)];
        }
        rms=static_cast<float>(std::sqrt(energy/(window+2)));
        const float floor=std::pow(10.0f,(-26.0f-0.46f*sensitivity)/20.0f);
        valid=false; confidence=0;
        if(rms<floor) return;
        double cumulative=0;
        difference[0]=1;
        for(int lag=1;lag<=maximumLag+1;++lag) {
            // Independent accumulators allow SIMD without unsafe fast-math.
            float s0=0,s1=0,s2=0,s3=0;
            for(int i=0;i<window;i+=4) {
                const auto d0=frame[static_cast<size_t>(i)]-frame[static_cast<size_t>(i+lag)];
                const auto d1=frame[static_cast<size_t>(i+1)]-frame[static_cast<size_t>(i+lag+1)];
                const auto d2=frame[static_cast<size_t>(i+2)]-frame[static_cast<size_t>(i+lag+2)];
                const auto d3=frame[static_cast<size_t>(i+3)]-frame[static_cast<size_t>(i+lag+3)];
                s0+=d0*d0; s1+=d1*d1; s2+=d2*d2; s3+=d3*d3;
            }
            const double sum=static_cast<double>(s0)+s1+s2+s3;
            rawDifference[static_cast<size_t>(lag)]=static_cast<float>(sum);
            cumulative+=sum;
            difference[static_cast<size_t>(lag)]=cumulative>1e-18 ? static_cast<float>(sum*lag/cumulative) : 1;
        }
        const float threshold=0.08f+0.0017f*sensitivity;
        int chosen=0;
        for(int lag=minimumLag;lag<=maximumLag;++lag) {
            if(difference[static_cast<size_t>(lag)]<threshold) {
                while(lag<maximumLag && difference[static_cast<size_t>(lag+1)]<difference[static_cast<size_t>(lag)]) ++lag;
                chosen=lag; break;
            }
        }
        if(chosen==0) return;
        const float left=rawDifference[static_cast<size_t>(chosen-1)], middle=rawDifference[static_cast<size_t>(chosen)];
        const float right=rawDifference[static_cast<size_t>(chosen+1)];
        const float curvature=left-2*middle+right;
        const float correction=std::abs(curvature)>1e-9f ? std::clamp(0.5f*(left-right)/curvature,-0.5f,0.5f) : 0;
        const float hz=static_cast<float>(rate/(chosen+correction));
        if(hz<54 || hz>1800) return;
        frequency=hz; confidence=1-difference[static_cast<size_t>(chosen)]; valid=true;
    }
    static constexpr int window=288;
    std::array<float,1024> ring{},frame{};
    std::array<float,256> difference{};
    std::array<float,256> rawDifference{};
    std::array<Biquad,2> antialias;
    int factor=4,maximumLag=219,minimumLag=6,position=0,collected=0,decimator=0,hopCount=0,hop=96;
    double rate=12000,dcCoefficient=0,dcInput=0,dcOutput=0;
};

class Ramp {
public:
    void reset(float v) { value=target=v; remaining=0; step=0; }
    void set(float v,int length) {
        if(std::abs(v-target)<1e-7f) return;
        target=v; remaining=std::max(1,length); step=(target-value)/static_cast<float>(remaining);
    }
    float next() {
        if(remaining>0 && --remaining==0) value=target;
        else if(remaining>0) value+=step;
        return value;
    }
private:
    float value=0,target=0,step=0;
    int remaining=0;
};

class Envelope {
public:
    void reset() { value=start=0; progress=0; open=false; }
    float tick(bool gate, float attackSeconds, float releaseSeconds, double sr) {
        if(gate!=open) { open=gate; start=value; progress=0; }
        if(progress<1) {
            progress=std::min(1.0,progress+1.0/(sr*(open ? attackSeconds : releaseSeconds)));
            const float shaped=static_cast<float>(progress*progress*(3-2*progress));
            value=start+((open ? 1.0f : 0.0f)-start)*shaped;
        }
        return value;
    }
    float value=0;
private:
    float start=0;
    double progress=0;
    bool open=false;
};

class Engine {
public:
    void prepare(double sampleRate) {
        sr=std::clamp(sampleRate,8000.0,384000.0);
        tracker.prepare(sr); pitchSmoothing=1-std::exp(-1/(0.012*sr));
        energySmoothing=1-std::exp(-1/(0.02*sr));
        levelSmoothing=1-std::exp(-1/(0.03*sr));
        reset();
    }
    void setParameters(Parameters next) {
        next.amount=bounded(next.amount,0,100,65);
        next.attackMs=bounded(next.attackMs,10,10000,700);
        next.releaseMs=bounded(next.releaseMs,10,15000,600);
        next.sensitivity=bounded(next.sensitivity,0,100,60);
        next.toneHz=bounded(next.toneHz,200,12000,6000);
        next.mix=bounded(next.mix,0,100,45); next.harmonic=std::clamp(next.harmonic,0,7);
        if(next.harmonic!=p.harmonic) selectedRatio=ratio(next.harmonic,targetPitch);
        p=next;
        const int smooth=static_cast<int>(sr*0.02);
        mix.set(p.mix/100,smooth); amount.set(p.amount/50,smooth);
        tone.set(std::log(p.toneHz),smooth);
    }
    void reset() {
        tracker.reset(); envelope.reset(); phase=0; trackedPitch=targetPitch=220;
        selectedRatio=ratio(p.harmonic,targetPitch); candidatePitch=0;
        energyLeft=energyRight=excitation=level=0; tone1=tone2=0;
        lostSeconds=0; stableFrames=0; tracking=false;
        dominant=0; toneCounter=0; retriggerSamples=0; wasBypassed=false;
        mix.reset(p.mix/100); amount.reset(p.amount/50); tone.reset(std::log(p.toneHz));
        toneCoefficient=1-std::exp(-2*pi*std::min(static_cast<double>(p.toneHz),sr*0.42)/sr);
    }
    void process(float* const* channels,int count,int frames) {
        if(count<1 || count>2) return;
        if(p.bypass) {
            if(!wasBypassed) { reset(); wasBypassed=true; }
            return;
        }
        wasBypassed=false;
        for(int i=0;i<frames;++i) {
            const float left=clean(channels[0][i]);
            const float right=count==2 ? clean(channels[1][i]) : left;
            energyLeft+=energySmoothing*(left*left-energyLeft);
            energyRight+=energySmoothing*(right*right-energyRight);
            if(dominant==0 && energyRight>energyLeft*1.5) dominant=1;
            if(dominant==1 && energyLeft>energyRight*1.5) dominant=0;
            if(tracker.push(dominant==0 ? left : right,p.sensitivity)) updateTracking();
            const bool restarting=retriggerSamples>0;
            const bool gate=p.engaged && tracking && !restarting;
            const float env=envelope.tick(gate,p.attackMs*0.001f,restarting ? 0.012f : p.releaseMs*0.001f,sr);
            if(restarting) --retriggerSamples;
            trackedPitch+=pitchSmoothing*(targetPitch-trackedPitch);
            level+=levelSmoothing*(excitation-level);
            const double frequency=trackedPitch*selectedRatio;
            phase+=frequency/sr; phase-=std::floor(phase);
            const double radians=2*pi*phase;
            double oscillator=std::sin(radians);
            if(frequency*2<sr*0.42) oscillator+=0.16*std::sin(radians*2);
            if(frequency*3<sr*0.42) oscillator+=0.045*std::sin(radians*3);
            if(frequency>sr*0.42) oscillator=0;
            const double drive=oscillator*(level*amount.next()/1.205);
            const double limited=drive/std::sqrt(1+drive*drive);
            const float toneLog=tone.next();
            if(--toneCounter<=0) {
                toneCounter=16;
                toneCoefficient=1-std::exp(-2*pi*std::min(std::exp(static_cast<double>(toneLog)),sr*0.42)/sr);
            }
            tone1+=toneCoefficient*(limited-tone1); tone2+=toneCoefficient*(tone1-tone2);
            const float blend=mix.next()*env;
            // No dry delay; the feedback blossoms out of the original note.
            if(blend>0) {
                const float wet=static_cast<float>(tone2);
                channels[0][i]=left*(1-blend)+wet*blend;
                if(count==2) channels[1][i]=right*(1-blend)+wet*blend;
            }
        }
    }
    float getEnvelope() const { return envelope.value; }
    float getPitch() const { return tracking ? static_cast<float>(targetPitch) : 0; }
    float getHarmonic() const { return static_cast<float>(selectedRatio); }
    bool isTracking() const { return tracking; }
private:
    static double ratio(int type,double pitch) {
        switch(type) {
            case 0: return pitch<220 ? 2 : 1; // Auto low: choose once per new note.
            case 1: return pitch<220 ? 5 : 3;
            case 2: return 0.5;
            case 3: return 1;
            case 4: return 2;
            case 5: return 3;
            case 6: return 4;
            case 7: return 5;
            default: return 1;
        }
    }
    void updateTracking() {
        const double dt=tracker.interval();
        if(tracker.valid) {
            const bool consistent=candidatePitch>0 && std::abs(std::log2(tracker.frequency/candidatePitch))<0.08;
            stableFrames=consistent ? std::min(stableFrames+1,1000) : 1;
            candidatePitch=tracker.frequency;
            const int needed=p.sensitivity>80 ? 2 : 3;
            if(stableFrames>=needed) {
                const bool newNote=!tracking || std::abs(std::log2(tracker.frequency/targetPitch))>0.125;
                if(newNote && tracking && envelope.value>0)
                    retriggerSamples=static_cast<int>(std::ceil(sr*0.012));
                if(newNote) selectedRatio=ratio(p.harmonic,tracker.frequency);
                if(!tracking && envelope.value<0.001f) trackedPitch=tracker.frequency;
                targetPitch=tracker.frequency; tracking=true; lostSeconds=0;
                excitation=std::max(std::min(0.8,static_cast<double>(tracker.rms)*1.414),excitation*std::exp(-dt/2.5));
                return;
            }
        } else { stableFrames=0; candidatePitch=0; }
        lostSeconds+=dt;
        const double tolerance=0.024+p.sensitivity*0.0012;
        if(lostSeconds>tolerance) tracking=false;
    }
    Parameters p;
    Tracker tracker;
    Envelope envelope;
    Ramp mix,amount,tone;
    double sr=48000,pitchSmoothing=0,energySmoothing=0,levelSmoothing=0;
    double phase=0,trackedPitch=220,targetPitch=220,selectedRatio=1,candidatePitch=0;
    double energyLeft=0,energyRight=0,excitation=0,level=0,tone1=0,tone2=0,toneCoefficient=0;
    double lostSeconds=0;
    int stableFrames=0,dominant=0,toneCounter=0,retriggerSamples=0;
    bool tracking=false,wasBypassed=false;
};
}
