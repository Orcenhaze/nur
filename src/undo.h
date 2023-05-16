#ifndef UNDO_H
#define UNDO_H

enum Action_Type
{
    ActionType_NONE,
    
    ActionType_PLAYER_MOVE,
    ActionType_OBJ_MOVE,
    ActionType_OBJ_ROTATE,
};

struct Player_Move
{
    s32 x, y;
    u8 dir;
};

struct Obj_Move
{
    s32 from_x, from_y;
    s32 to_x, to_y;
};

struct Obj_Rotate
{
    s32 x, y;
    u8 dir;
};

struct Undo_Action
{
    Action_Type type;
    
    union
    {
        Obj_Move obj_move;
        Player_Move player_move;
        Obj_Rotate obj_rotate;
    };
};

struct Undo_Record
{
    s32 actions_count;
    Undo_Action actions[8];
};

// @Note: Results in 292KB commit size.
#define MAX_RECORDS 1000
//
struct Undo_Handler
{
    s32 pending_actions_count;
    Undo_Action pending_actions[8];
    Array<Undo_Record> records;
};

FUNCTION void undo_handler_init(Undo_Handler *handler)
{
    handler->pending_actions_count = 0;
    MEMORY_ZERO_ARRAY(handler->pending_actions);
    array_init(&handler->records);
}

FUNCTION void undo_handler_reset(Undo_Handler *handler)
{
    // @Todo: Is calling this function sufficient when switching levels?
    //
    handler->pending_actions_count = 0;
    MEMORY_ZERO_ARRAY(handler->pending_actions);
    array_reset(&handler->records);
}

FUNCTION void undo_push_action(Undo_Handler *handler, Undo_Action new_action)
{
    if (handler->pending_actions_count < ARRAY_COUNT(handler->pending_actions)) {
        handler->pending_actions[handler->pending_actions_count] = new_action;
        handler->pending_actions_count++;
    }
}

FUNCTION void undo_push_player_move(Undo_Handler *handler, s32 x, s32 y, u8 dir)
{
    Undo_Action new_action;
    new_action.type = ActionType_PLAYER_MOVE;
    new_action.player_move = {x, y, dir};
    undo_push_action(handler, new_action);
}

FUNCTION void undo_push_obj_move(Undo_Handler *handler, s32 from_x, s32 from_y, s32 to_x, s32 to_y)
{
    Undo_Action new_action;
    new_action.type   = ActionType_OBJ_MOVE;
    new_action.obj_move = {from_x, from_y, to_x, to_y};
    undo_push_action(handler, new_action);
}

FUNCTION void undo_push_obj_rotate(Undo_Handler *handler, s32 obj_x, s32 obj_y, u8 dir_before)
{
    Undo_Action new_action;
    new_action.type   = ActionType_OBJ_ROTATE;
    new_action.obj_rotate = {obj_x, obj_y, dir_before};
    undo_push_action(handler, new_action);
}

FUNCTION void undo_end_frame(Undo_Handler *handler)
{
    s32 count = handler->pending_actions_count;
    if (!count) return;
    
    Undo_Record new_record; MEMORY_ZERO_STRUCT(&new_record);
    new_record.actions_count = count;
    memory_copy(new_record.actions, handler->pending_actions, count * sizeof(new_record.actions[0]));
    
    // @Note: If max records reached. Remove first half of all records.
    if (handler->records.count >= MAX_RECORDS) {
        array_remove_range(&handler->records, 0, (MAX_RECORDS/2)-1);
    }
    
    array_add(&handler->records, new_record);
    
    handler->pending_actions_count = 0;
    MEMORY_ZERO_ARRAY(handler->pending_actions);
}

FUNCTION void undo_perform_action(Undo_Action action)
{
    switch (action.type) {
        case ActionType_NONE: return;
        case ActionType_PLAYER_MOVE: {
            Player_Move c = action.player_move;
            set_player_position(c.x, c.y, c.dir);
        } break;
        case ActionType_OBJ_MOVE: {
            Obj_Move c = action.obj_move;
            // @Todo: Animate obj when undoing?
            //pushed_obj = v2(c.from_x, c.from_y);
            SWAP(objmap[c.to_y][c.to_x], objmap[c.from_y][c.from_x], Obj);
        } break;
        case ActionType_OBJ_ROTATE: {
            Obj_Rotate c = action.obj_rotate;
            objmap[c.y][c.x].dir = c.dir;
        } break;
    }
}

FUNCTION void undo_next(Undo_Handler *handler)
{
    if (handler->records.count <= 0)
        return;
    
    Undo_Record record = array_pop_back(&handler->records);
    for (s32 i = 0; i < record.actions_count; i++) {
        undo_perform_action(record.actions[i]);
    }
}

#endif //UNDO_H
