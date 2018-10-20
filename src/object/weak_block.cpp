//  SuperTux - Weak Block
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "object/weak_block.hpp"

#include <math.h>

#include "audio/sound_manager.hpp"
#include "badguy/badguy.hpp"
#include "math/random_generator.hpp"
#include "object/bullet.hpp"
#include "object/explosion.hpp"
#include "supertux/globals.hpp"
#include "supertux/sector.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "util/log.hpp"
#include "util/reader_mapping.hpp"

WeakBlock::WeakBlock(const ReaderMapping& lisp)
: MovingSprite(lisp, "images/objects/weak_block/strawbox.sprite", LAYER_TILES, COLGROUP_STATIC), state(STATE_NORMAL),
  linked(true),
  lightsprite(SpriteManager::current()->create("images/objects/lightmap_light/lightmap_light-small.sprite"))
{
  sprite->set_action("normal");
  //Check if this weakblock destroys adjacent weakblocks
  if(lisp.get("linked", linked)){
    if(! linked){
      sprite_name = "images/objects/weak_block/meltbox.sprite";
      sprite = SpriteManager::current()->create(sprite_name);
      sprite->set_action("normal");
    }
  }

  if (sprite_name == "images/objects/weak_block/strawbox.sprite") {
    lightsprite->set_blend(Blend::ADD);
    lightsprite->set_color(Color(0.3f, 0.2f, 0.1f));
    SoundManager::current()->preload("sounds/fire.ogg"); // TODO: use own sound?
  } else if (sprite_name == "images/objects/weak_block/meltbox.sprite") {
    SoundManager::current()->preload("sounds/sizzle.ogg");
  }
}

HitResponse
WeakBlock::collision_bullet(Bullet& bullet, const CollisionHit& hit)
{
  switch (state) {

    case STATE_NORMAL:
      //Ensure only fire destroys weakblock
      if(bullet.get_type() == FIRE_BONUS) {
        startBurning();
        bullet.remove_me();
      }
      //Other bullets ricochet
      else {
        bullet.ricochet(*this, hit);
      }
    break;

    case STATE_BURNING:
    case STATE_DISINTEGRATING:
      break;

    default:
      log_debug << "unhandled state" << std::endl;
      break;
	}

	return FORCE_MOVE;
}

HitResponse
WeakBlock::collision(GameObject& other, const CollisionHit& hit)
{
  switch (state) {

      case STATE_NORMAL:
        if (auto bullet = dynamic_cast<Bullet*> (&other)) {
          return collision_bullet(*bullet, hit);
        }
        //Explosions destroy weakblocks as well
        if (dynamic_cast<Explosion*> (&other)) {
          startBurning();
        }
        break;

      case STATE_BURNING:
        if(sprite_name != "images/objects/weak_block/strawbox.sprite")
          break;

        if(auto badguy = dynamic_cast<BadGuy*> (&other)) {
          badguy->ignite();
        }
        break;
      case STATE_DISINTEGRATING:
        break;

      default:
        log_debug << "unhandled state" << std::endl;
        break;
  }

  return FORCE_MOVE;
}

void
WeakBlock::update(float )
{
  switch (state) {

      case STATE_NORMAL:
        break;

      case STATE_BURNING:
        // cause burn light to flicker randomly
        if (linked) {
          if(gameRandom.rand(10) >= 7) {
            lightsprite->set_color(Color(0.2f + gameRandom.randf(20.0f) / 100.0f,
                                         0.1f + gameRandom.randf(20.0f)/100.0f,
                                         0.1f));
          } else
            lightsprite->set_color(Color(0.3f, 0.2f, 0.1f));
        }

        if (sprite->animation_done()) {
          state = STATE_DISINTEGRATING;
          sprite->set_action("disintegrating", 1);
          spreadHit();
          set_group(COLGROUP_DISABLED);
          lightsprite = SpriteManager::current()->create("images/objects/lightmap_light/lightmap_light-tiny.sprite");
          lightsprite->set_blend(Blend::ADD);
          lightsprite->set_color(Color(0.3f, 0.2f, 0.1f));
        }
        break;

      case STATE_DISINTEGRATING:
        if (sprite->animation_done()) {
          remove_me();
          return;
        }
        break;

  }
}

void
WeakBlock::draw(DrawingContext& context)
{
  //Draw the Sprite just in front of other objects
  sprite->draw(context.color(), get_pos(), LAYER_OBJECTS + 10);

  if (linked && (state != STATE_NORMAL))
  {
    lightsprite->draw(context.light(), bbox.get_middle(), 0);
  }
}

void
WeakBlock::startBurning()
{
  if (state != STATE_NORMAL) return;
  state = STATE_BURNING;
  sprite->set_action("burning", 1);
  if (sprite_name == "images/objects/weak_block/meltbox.sprite") {
    SoundManager::current()->play("sounds/sizzle.ogg");
  } else if (sprite_name == "images/objects/weak_block/strawbox.sprite") {
    SoundManager::current()->play("sounds/fire.ogg");
  }
}

void
WeakBlock::spreadHit()
{
  //Destroy adjacent weakblocks if applicable
  if(linked) {
    auto sector = Sector::current();
    for(auto& wb : sector->get_objects_by_type<WeakBlock>()) {
      if (&wb != this && wb.state == STATE_NORMAL)
      {
        const float dx = fabsf(wb.get_pos().x - bbox.p1.x);
        const float dy = fabsf(wb.get_pos().y - bbox.p1.y);
        if ((dx <= 32.5) && (dy <= 32.5)) {
          wb.startBurning();
        }
      }
    }
  }
}

ObjectSettings
WeakBlock::get_settings()
{
  ObjectSettings result = MovingSprite::get_settings();
  result.options.push_back( ObjectOption(MN_TOGGLE, _("Linked"), &linked,
                                         "linked"));

  return result;
}

/* EOF */
