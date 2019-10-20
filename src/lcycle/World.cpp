#include "World.hpp"

#include <utility>
#include <vector>
#include <algorithm>
#include <set>
#include <map>

#include <mathfu/glsl_mappings.h>

#include "Cycle.hpp"


namespace lcycle {

World::World()
    : _players(), _trails(), _size(0.0), _dashTime(0.0),
      _curTime(0.0), _drawing(false) {}

World::World(double size, double dashTime, const std::vector<Player>& players)
    : _players(players),
      _trails(_players.size()), _size(size), _dashTime(dashTime),
      _curTime(0.0), _drawing(false)
{
    for(size_t i=0; i<_trails.size(); i++) {
        _trails[i].color() = _players[i].tColor;
    }
}

void World::runFor(double secs, const std::vector<std::pair<int, CycleInput>>& inputs) {
    using namespace mathfu;

    std::map<int, int> livingPlayers;
    for(auto i = 0u; i < _players.size(); i++) {
        livingPlayers.insert({_players[i].id, i});
    }
    std::vector<std::pair<int, CycleInput>> adjustedInputs;
    for(const auto& input : inputs) {
        if (livingPlayers.count(input.first) > 0) {
            adjustedInputs.push_back({livingPlayers[input.first], input.second});
        }
    }

    //update cycle positions
    for(const auto& input : adjustedInputs) {
        auto& player = _players[input.first];
        player.cycle.rotate( -1 * TURN_SPEED * secs * input.second.turnDir);
        player.cycle.runFor(secs);
    }

    _curTime += secs;
    if(_drawing && _curTime > 2 * _dashTime) {
        _drawing = false;
    } else if(_drawing) {
        for(const auto& input : adjustedInputs) {
            auto i = input.first;
            auto coord = _players[i].cycle.toLine().start();
            _trails[i][_trails[i].size()-1].end() = coord;
        }
    } else if(_curTime > _dashTime) {
        for(const auto& input : adjustedInputs) {
            auto i = input.first;
            auto coord = _players[i].cycle.toLine().start();
            _trails[i].add(Line(coord, coord));
        }
        _drawing = true;
    }
    _curTime = fmod(_curTime, 2 * _dashTime);


    //players that died this frame, RIP
    std::set<size_t> kill;

    std::vector<Line> cycLines;
    cycLines.reserve(_players.size());
    for(auto& player : _players) {
        cycLines.push_back(player.cycle.toLine());
    };

    //check for collisions against the edges
    float sizeDiv2 = _size / 2;
    for(size_t i=0; i<cycLines.size(); i++) {
        auto& line = cycLines[i];
        if(!InRange2D(line.start(), vec2(-sizeDiv2, -sizeDiv2), vec2(sizeDiv2, sizeDiv2))
           || !InRange2D(line.end(), vec2(-sizeDiv2, -sizeDiv2), vec2(sizeDiv2, sizeDiv2))) {
            kill.insert(i);
        }
    }

    //check for cycle-cycle collisions
    for(size_t i=0; i<cycLines.size(); i++) {
        auto& first = cycLines[i];
        for(size_t j=i+1; j<cycLines.size(); j++) {
                auto& second = cycLines[j];
                if(Line::intersect(first, second)) {
                    kill.insert({i, j});
                }
        }
    }

    //check for cycle-trail collisions
    for(auto& trail : _trails) {
        for(auto& line : trail.data()) {
            for(size_t i=0; i<cycLines.size(); i++) {
                if(Line::intersect(cycLines[i], line)) {
                    kill.insert(i);
                }
            }
        }
    }

    //remove dead players
    for(auto i=kill.rbegin(); i!=kill.rend(); ++i) {
            std::swap(_players[*i], _players[_players.size() - 1]);
            std::swap(_trails[*i], _trails[_trails.size() - 1]);
            _players.pop_back();
            //notice that trails aren't removed
    }
}

const std::vector<Player>& World::players() const {
    return _players;
}

const std::vector<Trail>& World::trails() const {
    return _trails;
}

double World::size() const {
    return _size;
}

}

