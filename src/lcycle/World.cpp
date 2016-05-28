#include "World.hpp"

#include <utility>
#include <vector>
#include <algorithm>
#include <set>

#include <mathfu/glsl_mappings.h>

#include "Cycle.hpp"


namespace lcycle {

World::World(double size, double dashTime, std::vector<Player>&& players)
    : _players(players), _trails(_players.size()), _size(size),
      _dashTime(dashTime), _curTime(0.0), _drawing(false)
{
    for(size_t i=0; i<_trails.size(); i++) {
        _trails[i].color() = _players[i].tColor;
    }
}

World::World(World&& o)
    : _players(o._players), _trails(o._trails), _size(o._size),
      _dashTime(o._dashTime), _curTime(o._curTime), _drawing(o._drawing)
{}

World& World::operator=(World&& o) {
    _players = o._players;
    _trails = o._trails;
    _size = o._size;
    _dashTime = o._dashTime;
    _curTime = o._curTime;
    _drawing = o._drawing;
    return *this;
}

void World::runFor(double secs) {
    using namespace mathfu;

    //update cycle positions
    for(auto& player : _players) {
        player.cycle.rotate( -1 * TURN_SPEED * secs * player.input().turnDir);
        player.cycle.runFor(secs);
    }

    _curTime += secs;
    if(_drawing && _curTime > 2 * _dashTime) {
        _drawing = false;
    } else if(_drawing) {
        for(size_t i=0; i<_players.size(); i++) {
            auto coord = _players[i].cycle.toLine().start();
            _trails[i][_trails[i].size()-1].end() = coord;
        }
    } else if(_curTime > _dashTime) {
        for(size_t i=0; i<_players.size(); i++) {
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

    //build a list of all the trails and do cycle-trail collision
    std::vector<Line> allLines;
    size_t total = 0;
    for(auto& trail : _trails) {
        total += trail.size();
    }
    allLines.reserve(total);

    for(auto& trail : _trails) {
        allLines.insert(allLines.end(), trail.data().cbegin(), trail.data().cend());
    }

    for(size_t i=0; i<cycLines.size(); i++) {
        for(auto& line : allLines) {
            if(Line::intersect(cycLines[i], line)) {
                kill.insert(i);
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

