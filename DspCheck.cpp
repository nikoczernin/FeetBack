#include "Engine.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
void check(bool ok,const char* description) { if(!ok) throw std::runtime_error(description); }
double energy(const std::vector<float>& x,size_t start=0) {
    double total=0; for(size_t i=start;i<x.size();++i) total+=x[i]*x[i];
    return std::sqrt(total/static_cast<double>(x.size()-start));
}
std::vector<float> note(double sr,double hz,double seconds,bool rich=false) {
    std::vector<float> data(static_cast<size_t>(sr*seconds));
    for(size_t i=0;i<data.size();++i) {
        const double a=2*feetback::pi*hz*static_cast<double>(i)/sr;
        data[i]=static_cast<float>(rich ? 0.04*std::sin(a)+0.16*std::sin(2*a)+0.09*std::sin(3*a) : 0.2*std::sin(a));
    }
    return data;
}
void process(feetback::Engine& e,std::vector<float>& data,int block=127) {
    for(size_t i=0;i<data.size();i+=static_cast<size_t>(block)) {
        float* channels[]{data.data()+i};
        e.process(channels,1,static_cast<int>(std::min(data.size()-i,static_cast<size_t>(block))));
    }
}
double spectral(const std::vector<float>& x,double sr,double hz,size_t start) {
    double re=0,im=0;
    for(size_t i=start;i<x.size();++i) {
        const double angle=2*feetback::pi*hz*static_cast<double>(i)/sr;
        re+=x[i]*std::cos(angle); im+=x[i]*std::sin(angle);
    }
    return 2*std::hypot(re,im)/static_cast<double>(x.size()-start);
}
}
int main() {
    try {
        double worstCents=0;
        for(double sr:{11025.0,22050.0,44100.0,48000.0,96000.0,192000.0}) {
            for(double hz:{55.0,65.406,82.407,110.0,196.0,329.628,440.0,880.0,1318.51}) {
                feetback::Engine e; feetback::Parameters p; p.attackMs=10; p.mix=100; p.harmonic=3;
                e.setParameters(p); e.prepare(sr);
                auto x=note(sr,hz,0.35); process(e,x);
                const double cents=std::abs(1200*std::log2(e.getPitch()/hz));
                if(!e.isTracking() || cents>25) std::cerr<<"Pitch: "<<sr<<" Hz, input "<<hz<<", tracked "<<e.getPitch()<<", cents "<<cents<<'\n';
                check(e.isTracking() && cents<25,"sine pitch tracking"); worstCents=std::max(worstCents,cents);
                for(auto sample:x) check(std::isfinite(sample) && std::abs(sample)<=1,"bounded finite output");
            }
            feetback::Engine rich; rich.prepare(sr);
            auto overtones=note(sr,110,0.4,true); process(rich,overtones);
            check(std::abs(rich.getPitch()-110)<1,"strong overtones must not cause octave error");
        }
        std::cout<<"Pitch checks passed; worst sine error "<<worstCents<<" cents.\n";
        for(int type=2;type<8;++type) {
            feetback::Engine e; feetback::Parameters p; p.harmonic=type; p.mix=100; p.attackMs=10; p.toneHz=12000;
            e.setParameters(p); e.prepare(48000);
            auto x=note(48000,196,1.5); process(e,x);
            const double ratio=std::array<double,6>{0.5,1,2,3,4,5}[static_cast<size_t>(type-2)];
            const auto expected=spectral(x,48000,196*ratio,24000);
            check(expected>0.08,"harmonic output spectral peak");
            if(ratio!=1) check(expected>spectral(x,48000,196,24000)*5,"wet output must predominantly contain selected harmonic");
        }
        std::cout<<"All six fixed harmonic spectra passed.\n";
        {
            feetback::Engine e; feetback::Parameters p; p.attackMs=500; e.setParameters(p); e.prepare(48000);
            auto first=note(48000,110,1); process(e,first);
            check(e.getEnvelope()==1,"first note reaches full feedback");
            auto next=note(48000,330,0.14); process(e,next);
            check(std::abs(e.getPitch()-330)<2 && e.getEnvelope()<0.3,"new note restarts the swell");
        }
        {
            feetback::Engine e; feetback::Parameters p; p.mix=100; p.attackMs=300; p.releaseMs=400;
            e.setParameters(p); e.prepare(48000);
            auto x=note(48000,220,1); process(e,x);
            check(e.getEnvelope()==1,"attack reaches full feedback");
            p.engaged=false; e.setParameters(p);
            auto first=note(48000,220,0.2); process(e,first);
            check(std::abs(e.getEnvelope()-0.5)<0.015,"release midpoint timing");
            auto rest=note(48000,220,0.25); const auto dry=rest; process(e,rest);
            check(e.getEnvelope()==0,"release completes");
            check(std::memcmp(rest.data()+11000,dry.data()+11000,(rest.size()-11000)*sizeof(float))==0,"release returns exact dry");
            p.engaged=true; e.setParameters(p);
            auto attack=note(48000,220,0.15); process(e,attack);
            check(std::abs(e.getEnvelope()-0.5)<0.015,"attack midpoint timing");
            std::vector<float> silence(48000); process(e,silence);
            check(!e.isTracking() && e.getEnvelope()==0,"muting ends feedback");
        }
        {
            std::mt19937 random(27); std::uniform_real_distribution<float> sample(-0.1f,0.1f);
            feetback::Engine e; e.prepare(48000); std::vector<float> noise(96000);
            for(auto& x:noise) x=sample(random);
            const auto original=noise; process(e,noise);
            check(!e.isTracking() && e.getEnvelope()==0,"noise must not trigger sustained feedback");
            check(std::memcmp(noise.data(),original.data(),noise.size()*sizeof(float))==0,"untracked noise stays dry");
            feetback::Parameters p; p.bypass=true; p.amount=100; p.mix=100; e.setParameters(p);
            auto dry=note(48000,220,0.5); const auto reference=dry; process(e,dry,17);
            check(std::memcmp(dry.data(),reference.data(),dry.size()*sizeof(float))==0,"sample-exact bypass");
            p.bypass=false; p.mix=0; e.setParameters(p); e.reset();
            process(e,dry); check(std::memcmp(dry.data(),reference.data(),dry.size()*sizeof(float))==0,"sample-exact dry mix");
        }
        {
            feetback::Engine e; e.prepare(48000);
            double phase=0; std::vector<float> bend(96000);
            for(size_t i=0;i<bend.size();++i) {
                const auto hz=220*std::pow(2.0,2.0*static_cast<double>(i)/(12*bend.size()));
                phase+=2*feetback::pi*hz/48000; bend[i]=static_cast<float>(0.2*std::sin(phase));
            }
            process(e,bend);
            check(std::abs(1200*std::log2(e.getPitch()/246.942))<12,"whole-tone bend tracking");
            auto left=note(48000,110,0.5),right=left; for(auto& x:right) x=-x;
            e.reset(); float* stereo[]{left.data(),right.data()}; e.process(stereo,2,static_cast<int>(left.size()));
            check(std::abs(e.getPitch()-110)<1,"anti-phase stereo must still track");
        }
        {
            std::array<double,2> levels{};
            for(size_t i=0;i<2;++i) {
                feetback::Engine e; feetback::Parameters p; p.harmonic=6; p.mix=100; p.attackMs=10; p.toneHz=i==0 ? 200.0f : 12000.0f;
                e.setParameters(p); e.prepare(48000); auto x=note(48000,330,0.7); process(e,x);
                levels[i]=energy(x,24000);
            }
            check(levels[1]>levels[0]*15,"tone dark/bright response");
        }
        {
            auto input=note(48000,110,4,true); feetback::Engine a,b; a.prepare(48000); b.prepare(48000);
            auto one=input,two=input; process(a,one,1); process(b,two,1024);
            check(one==two,"host block size must not change the sound");
            const auto started=std::chrono::steady_clock::now();
            feetback::Engine bench; bench.prepare(48000); process(bench,input,64);
            const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
            std::cout<<"48 kHz mono engine: "<<seconds/4*100<<"% of one CPU core for this run.\n";
        }
        std::cout<<"PASS: pitch, harmonics, attack/release, noise rejection, silence, bypass, bends, stereo, tone, buffer invariance.\n";
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
