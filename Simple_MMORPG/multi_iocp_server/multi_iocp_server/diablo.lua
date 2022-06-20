my_id = 9999

function set_object_id (id)
my_id = id
end

function event_boss_skill(playerid, skillindex)
	newSkillidx = skillindex + 1;

	if (newSkillidx <= 2) then
		API_Chat(playerid, my_id, newSkillidx);
	end
	if (newSkillidx == 3) then
		Boss_Skill(playerid, my_id, newSkillidx);
	end
	
end


function event_boss_move(dir)
	my_x = API_get_x(my_id)
	my_y = API_get_y(my_id)

	new_x = my_x;
	new_y = my_y;

	if (dir == 0) then
		new_y = my_y - 1;
	end
	if (dir == 1) then
		if (my_y < 1999) then
			new_y = my_y + 1;
		end
	end
	if (dir == 2) then
		new_x = my_x - 1;
	end
	if (dir == 3) then
		if (my_x < 1999) then
			new_x = my_x + 1;
		end
	end

	if (new_x < 0) then
		new_x = 0;
	end
	if (new_y < 0) then
		new_y = 0;
	end
	Boss_Move(my_id, new_x, new_y);
end
