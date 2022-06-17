my_id = 9999

function set_object_id (id)
my_id = id
end

function event_player_move(player_id)
	player_x = API_get_x(player_id)
	player_y = API_get_y(player_id)
	my_x = API_get_x(my_id)
	my_y = API_get_y(my_id)
	if (player_x == my_x) then
		if (player_y == my_y) then
			API_chat(player_id, my_id, "HELLO");
		end
	end
end

function event_npc_run(player_id, dir, runIndex)
	my_x = API_get_x(my_id)
	my_y = API_get_y(my_id)
	newRunIndex = runIndex + 1
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
	API_run(player_id, my_id, new_x, new_y, newRunIndex);
end
