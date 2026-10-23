/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
FPS sweep: for each backend and agent count, runs a fixed number of frames
with exactly one simulation step per frame (no frame cap, no vsync) and
records the per-phase frame breakdown. Used for the report items "largest
agent count at 30 and 60 FPS" and "cost of the per-frame GPU -> host copy".
*/

#include "app/Application.h"

#include "bench/MetricsLogger.h"
#include "bench/Timer.h"

#include <iomanip>
#include <iostream>
#include <map>

namespace swarm
{

int Application::runFpsSweep()
{
    MetricsLogger csv;
    const std::vector<std::string> header{
        "timestamp", "host", "backend", "device", "threads", "agents", "shape", "frames",
        "frame_ms_mean", "frame_ms_p95", "fps_mean", "step_ms_mean", "copy_ms_mean", "copy_ms_p95",
        "fill_ms_mean", "draw_ms_mean", "copy_pct_of_frame"};
    if (!csv.open(m_config.sweepOut, header, true))
    {
        return 1;
    }

    std::vector<std::string> backends = m_config.sweepBackends;
    if (backends.empty())
    {
        backends.push_back(m_sim->backendName());
    }

    // Largest count reaching each FPS target, per backend.
    std::map<std::string, std::size_t> best30;
    std::map<std::string, std::size_t> best60;

    std::cout << std::fixed << std::setprecision(2);
    const double antFrac = m_config.spawn.total() ? static_cast<double>(m_config.spawn.ants) /
                                                        static_cast<double>(m_config.spawn.total()) : 0.0;
    const double predFrac = m_config.spawn.total() ? static_cast<double>(m_config.spawn.predators) /
                                                         static_cast<double>(m_config.spawn.total()) : 0.0;

    for (const std::string& backendName : backends)
    {
        if (!m_sim->switchBackend(backendName))
        {
            continue;
        }
        for (std::size_t count : m_config.sweepCounts)
        {
            SpawnConfig spawn = m_config.spawn;
            spawn.ants = static_cast<std::size_t>(static_cast<double>(count) * antFrac);
            spawn.predators = static_cast<std::size_t>(static_cast<double>(count) * predFrac);
            spawn.boids = count - spawn.ants - spawn.predators;
            m_sim->reset(spawn);

            std::vector<double> frame, step, copy, fill, draw;
            const int total = m_config.sweepWarmupFrames + m_config.sweepFrames;
            for (int f = 0; f < total && m_window.isOpen() && !m_view.quit; ++f)
            {
                Stopwatch sw;
                m_sim->beginFrame();
                processEvents();
                m_sim->step(m_config.fixedDt);
                renderFrame();
                const double frameMs = sw.elapsedMs();

                if (f >= m_config.sweepWarmupFrames)
                {
                    const FrameTimings& t = m_sim->timings();
                    frame.push_back(frameMs);
                    step.push_back(t.stepMs);
                    copy.push_back(t.copyMs);
                    fill.push_back(t.fillMs);
                    draw.push_back(t.drawMs);
                }
            }
            if (m_view.quit || frame.empty())
            {
                std::cout << "sweep aborted\n";
                return 1;
            }

            const SampleStats fs = summarize(frame);
            const SampleStats cs = summarize(copy);
            const double fps = fs.mean > 0.0 ? 1000.0 / fs.mean : 0.0;
            const double copyPct = fs.mean > 0.0 ? 100.0 * cs.mean / fs.mean : 0.0;

            csv.add(timestampNow()).add(hostName()).add(m_sim->backendName()).add(m_sim->deviceName())
                .add(m_sim->threadCount()).add(count).add(m_renderer.shapeName()).add(fs.count)
                .add(fs.mean).add(fs.p95).add(fps, 2).add(summarize(step).mean).add(cs.mean).add(cs.p95)
                .add(summarize(fill).mean).add(summarize(draw).mean).add(copyPct, 2);
            csv.endRow();

            std::cout << m_sim->backendName() << "  agents=" << count << "  " << fps << " FPS  (frame "
                      << fs.mean << " ms: step " << summarize(step).mean << ", copy " << cs.mean << ", fill "
                      << summarize(fill).mean << ", draw " << summarize(draw).mean << ")\n";

            if (fps >= 30.0) best30[backendName] = std::max(best30[backendName], count);
            if (fps >= 60.0) best60[backendName] = std::max(best60[backendName], count);
        }
    }

    std::cout << "\nLargest tested agent count reaching the target (mean FPS):\n";
    for (const std::string& b : backends)
    {
        std::cout << "  " << b << ":  30 FPS -> " << (best30.count(b) ? std::to_string(best30[b]) : "none")
                  << "   60 FPS -> " << (best60.count(b) ? std::to_string(best60[b]) : "none") << '\n';
    }
    std::cout << "appended to " << m_config.sweepOut << '\n';
    m_window.close();
    return 0;
}

}  // namespace swarm
