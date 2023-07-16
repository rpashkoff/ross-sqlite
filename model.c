#include "model.h"


tw_peid model_map(tw_lpid gid);
void model_init(model_state_t * s, tw_lp * lp);
void model_handler(model_state_t * s, tw_bf * bf, model_message_t * m, tw_lp * lp);
void model_handler_rc(model_state_t * s, tw_bf * bf, model_message_t * m, tw_lp * lp);
void model_finish(model_state_t * s, tw_lp * lp);



void db_init(void ** _db, void * lp, void * mem_pool_ptr, size_t mem_pool_size, const char * db_filename, const char * init_script_filename);
void db_exec_from_file(void * db, void * lp, const char * filename);
int db_exec(void *db, void * lp, const char * sql);

void db_dec_action(void * db, void * lp, int id);
void db_inc_action(void * db, void * lp, int id);
void db_print_row(void * db, void * lp, int id);



unsigned int log_level = LOG_INFO;

tw_lptype model_lps[] =
{
    {   (init_f) model_init,
        (pre_run_f) NULL,
        (event_f) model_handler,
        (revent_f) model_handler_rc,
        (commit_f) NULL,
        (final_f) model_finish,
        (map_f) model_map,
        sizeof(model_state_t)},
    {0},
};



tw_peid model_map(tw_lpid gid) {
    return (tw_peid) gid / g_tw_nlp;
}


void send_event(model_state_t * s, tw_lp * lp, int delta_t, model_message_t * message) {
    int self = lp->gid;
    tw_event * e = tw_event_new(self, delta_t, lp);
    model_message_t * m = tw_event_data(e);
    *m = *message;
    m->sender = self;
    tw_event_send(e);
}


void model_init(model_state_t * s, tw_lp * lp) {
    db_init(&(s->db), lp, s->mem_pool, MEM_POOL_SIZE,  "./ross-sqlite.db" , "dat/db_init.sql");
    db_exec_from_file(s->db, lp, "dat/db_fill.sql");
    model_message_t m;
    memset(&m, 0, sizeof(model_message_t));
    m.type = EVENT_INC;
    send_event(s, lp, 10, &m);
    log_info(lp, "Model init completed");
}


void model_handler(model_state_t * s, tw_bf * bf, model_message_t * m, tw_lp * lp) {
    switch(m->type) {
        case EVENT_INC: {
            db_inc_action(s->db, lp, 1);
            model_message_t m;
            memset(&m, 0, sizeof(model_message_t));
            m.type = EVENT_DEC;
            send_event(s, lp, 20, &m);
            break;
        }
        case EVENT_DEC: {
            db_dec_action(s->db, lp, 2);
            break;
        }
    }
}


void model_handler_rc(model_state_t * s, tw_bf * bf, model_message_t * m, tw_lp * lp) {}


void model_finish(model_state_t * s, tw_lp * lp) {
    db_print_row(s->db, lp, 1);
    db_print_row(s->db, lp, 2);
    log_info(lp, "Model is finished");
}


const tw_optdef app_opt [] = {
        TWOPT_GROUP("Sample Model Params"),
        TWOPT_UINT("log-level", log_level, "Log level (1-5)"),
        TWOPT_END()
};


int main(int argc, char **argv, char **env) {
    int i;
    int num_lps_per_pe;

    tw_opt_add(app_opt);
    tw_init(&argc, &argv);

    num_lps_per_pe = 1;
    g_tw_events_per_pe = 16384;

    tw_define_lps(num_lps_per_pe, sizeof(model_message_t));

    g_tw_lp_types = model_lps;
    tw_lp_setup_types();

    log_set_level(log_level);

    tw_run();
    tw_end();

    return 0;
}
