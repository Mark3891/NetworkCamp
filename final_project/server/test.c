void *handle_client(void *arg)
{
    ThreadArgs *client = (ThreadArgs *)arg;
    int ready;

    // 여기서 Mutex때문에 다른 애들에 Ready를 받지 못한다.
    while (1)
    {
        // pthread_mutex_lock(&mutex);
        for (int i = 0; i < board->room_height; i++)
        {
            if (read(client->clnt_sock, game->board[i], sizeof(int) * board->room_width) <= 0)
            {
                perror("read board error");
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }

        if (read(client->clnt_sock, game->players, sizeof(player) * board->player_number) <= 0)
        {
            perror("read players error");
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        // pthread_mutex_unlock(&mutex);

        for (int i = 0; i < board->room_height; i++)
        {
            for (int j = 0; j < board->room_width; j++)
            {
                printf("%d ", game->board[i][j]);
            }
            printf("\n");
        }

        for (int i = 0; i < board->player_number; i++)
        {
            printf("Index : %d\n", game->players[i].index);
            printf("Player ID: %d\n", game->players[i].player_id);
            printf("Player Team: %d\n", game->players[i].team);
            printf("Player Initial Position: (%d, %d)\n", game->players[i].x, game->players[i].y);
        }

        // pthread_mutex_lock(&mutex);
        for (int i = 0; i < expected_clients; i++)
        {
            for (int j = 0; j < board->room_height; j++)
            {
                write(p[i].player_id, game->board[j], sizeof(int) * board->room_width);
            }
            write(p[i].player_id, game->players, sizeof(player) * expected_clients);
        }
        // pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

// str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);

                // if (str_len == 0)
                // {
                //     epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                //     close(ep_events[i].data.fd);
                //     printf("closed client: %d \n", ep_events[i].data.fd);
                //     connected_clients--;
                // }