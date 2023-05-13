#include "include/simulate.hpp"
#include "include/template.hpp"
#include "include/common.hpp"
#include <python3.9/Python.h>
#include "include/self/debug"
#include <bits/stdc++.h>
// #define mp make_pair
// #define pii pair<int,int>
using namespace std;
const int N=MAP_SIZE;

int tower[N][N];
int ant_max_hp[2][N][N];
int ant_sum_hp[2][N][N];
int tower_id[105];
TowerType id_tower[15];
// auto seed=chrono::steady_clock().now().time_since_epoch().count();
ull seed=20050114;
mt19937 rng(seed);
vector<pii> towerPos[2];
PyObject* pFunc;
int isTraining;

struct Board
{
	int player_id;
	GameInfo info;
	GameState gameState;
	Board() {}
	Board(int player_id,GameInfo info):
		player_id(player_id),info(info),gameState(Running){}
	int convertToOperation(int op,vector<Operation> &vec)const{
		vec.clear();
		if(op==0) {}
		else if(op<towerPos[player_id].size()*4+1)
		{
			op--;
			int x=towerPos[player_id][op/4].first;
			int y=towerPos[player_id][op/4].second;
			int type=op%4;
			int found_type=0,found_id=-1;
			for(auto &tower:info.all_towers())
			{
				if(tower.player!=player_id) continue;
				if(tower.x!=x || tower.y!=y) continue;
				found_type=tower_id[tower.type];
				found_id=tower.id;
			}
			if(type==0)
			{
				if(found_type==0) return false;
				else
				{
					vec.emplace_back(Operation(DowngradeTower,found_id,-1));
				}
			}
			else
			{
				if(found_type>=5) return false;
				else
				{
					if(found_type==0)
					{
						vec.emplace_back(Operation(BuildTower,x,y));
					}
					else
					{
						vec.emplace_back(Operation(UpgradeTower,found_id,id_tower[found_type*3+type-2]));
					}
				}
			}
		}
		for(int i=0;i<(int)vec.size();i++)
		{
			auto tmp=vec;
			tmp.resize(i);
			if(!info.is_operation_valid(player_id,tmp,vec[i])) return false;
		}
		return true;
	}
	int isValid(int x)const
	{
		vector<Operation> tmp;
		return convertToOperation(x,tmp);
	}
	Board getMove(int x)const
	{
		vector<Operation> vec;
		Board board(*this);
		Simulator s(board.info);
		if(convertToOperation(x,vec)) 
		{
			for(auto &op:vec)
			{
				s.add_operation_of_player(board.player_id,op);
			}
			s.apply_operations_of_player(board.player_id);
		}
		else
		{
			assert(0);
			//do nothing
		}
		if(board.player_id==1) board.gameState=s.next_round();
		board.info=s.get_info();
		board.player_id^=1;
		return board;
	}
};

namespace MyAI
{
	using ftype=double;
	const int ActionSize=133;
	const int M=50;
	const ftype cpuct=1;
	vector<ftype> paddingBoard(const Board &board)
	{
		auto player_id=board.player_id;
		auto info=board.info;
		memset(tower,0,sizeof(tower));
		memset(ant_sum_hp,0,sizeof(ant_sum_hp));
		memset(ant_max_hp,0,sizeof(ant_max_hp));
		for(auto &ant:info.all_ants())
		{
			ant_sum_hp[ant.player][ant.x][ant.y]+=ant.hp;
			ant_max_hp[ant.player][ant.x][ant.y]=max(ant_max_hp[ant.player][ant.x][ant.y],ant.hp);
		}
		vector<ftype> vec;
		int cnt0=0,cnt1=0;
		for(int i=0;i<MAP_SIZE;i++)
		{
			for(int j=0;j<MAP_SIZE;j++)
			{
				if(MAP_PROPERTY[i][j]==-1) continue;
				if(MAP_PROPERTY[i][j]==1) continue;
				int y=j;
				int x=(j&1)?18-i:17-i;
				if(MAP_PROPERTY[i][j]==0) assert(MAP_PROPERTY[x][y]==0);
				if(MAP_PROPERTY[i][j]==2) assert(MAP_PROPERTY[x][y]==3);
				if(MAP_PROPERTY[i][j]==3) assert(MAP_PROPERTY[x][y]==2);
				if(player_id==0) x=i,y=j;
				if(MAP_PROPERTY[x][y]==0)
				{
					vec.emplace_back(info.pheromone[player_id^1][x][y]);
					vec.emplace_back(ant_sum_hp[player_id^1][x][y]);
					vec.emplace_back(ant_max_hp[player_id^1][x][y]);
					vec.emplace_back(info.pheromone[player_id][x][y]);
					vec.emplace_back(ant_sum_hp[player_id][x][y]);
					vec.emplace_back(ant_max_hp[player_id][x][y]);
				}
				else if(MAP_PROPERTY[x][y]==(2^player_id))
				{
					for(int k=0;k<14;k++)
					{
						if(tower[x][y]==k) vec.emplace_back(1);
						else vec.emplace_back(0);
					}
					if(tower[x][y]) cnt0++;
				}
				else if(MAP_PROPERTY[x][y]==(3^player_id))
				{
					for(int k=0;k<14;k++)
					{
						if(tower[x][y]==k) vec.emplace_back(1);
						else vec.emplace_back(0);
					}
					if(tower[x][y]) cnt1++;
				}
			}
		}
		for(int k=0;k<8;k++)
		{
			if(cnt0==k) vec.emplace_back(1);
			else vec.emplace_back(0);
		}
		for(int k=0;k<8;k++)
		{
			if(cnt1==k) vec.emplace_back(1);
			else vec.emplace_back(0);
		}
		vec.emplace_back(info.coins[player_id]);
		vec.emplace_back(info.coins[player_id^1]);
		vec.emplace_back(info.bases[player_id].hp);
		vec.emplace_back(info.bases[player_id^1].hp);
		for(int k=0;k<9;k++)
		{
			if(info.bases[player_id].gen_speed_level==k/3 && info.bases[player_id].ant_level==k%3) vec.emplace_back(1);
			else vec.emplace_back(0);
		}
		for(int k=0;k<9;k++)
		{
			if(info.bases[player_id^1].gen_speed_level==k/3 && info.bases[player_id^1].ant_level==k%3) vec.emplace_back(1);
			else vec.emplace_back(0);
		}
		// cout<<vec.size()<<endl;
		return vec;
	}
	vector<ftype> predictPY(const Board &board,int valid[],int applyDir)
	{
		vector<ftype> seq=paddingBoard(board);
		vector<ftype> vvalid;
		for(int i=0;i<ActionSize;i++) vvalid.emplace_back(valid[i]);
		PyObject *list1=PyList_New(0),*list2=PyList_New(0);
		// cerr<<seq.size()<<' ';
		// cout<<endl;
		for(int i=0;i<seq.size();i++)
		{
			// cerr<<"Building"<<' '<<i<<' '<<(float)seq[i]<<endl;
			auto tmp=Py_BuildValue("f",(float)seq[i]);
			// cerr<<"Built"<<endl;
			assert(tmp);
			PyList_Append(list1,tmp);
			// Py_DecRef(tmp);
		}
		// cerr<<"Predicting"<<endl;
		for(int i=0;i<ActionSize;i++)
		{
			auto tmp=Py_BuildValue("f",(float)valid[i]);
			assert(tmp);
			PyList_Append(list2,tmp);
			// Py_DecRef(tmp);
		}
		assert(pFunc);
		assert(list1);assert(list2);
		// cerr<<"FUCK"<<endl;
		// cerr<<seq.size()<<' '<<ActionSize<<endl;
		PyObject* result=PyObject_CallFunction(pFunc,"(OOi)",list1,list2,applyDir);
		assert(result);
		// cout<<result<<endl;
		// cerr<<"SHIT"<<endl;
		vector<ftype> res;
		for(int i=0;i<1+ActionSize;i++)
		{
			PyObject* tmp=PyList_GetItem(result,i);
			assert(tmp);
			res.emplace_back(PyFloat_AsDouble(tmp));
			// Py_DecRef(tmp);
		}
		Py_DecRef(list1);
		Py_DecRef(list2);
		Py_DecRef(result);
		// for(auto x:res) cout<<x<<' ';
		// cout<<endl;
		// exit(0);
		return res;
	}
	int randomChoose(const vector<ftype> &v)
	{
		assert(v.size());
		ftype tot=0;
		for(auto &x:v) tot+=x;
		assert(tot>1e-6);
		ftype x=uniform_real_distribution<ftype>(0,tot)(rng);
		int ch=-1;
		for(int i=0;i<(int)v.size();i++)
		{
			if(x<v[i]) return i;
			x-=v[i];
			if(v[i]>0) ch=i;
		}
		assert(ch!=-1);
		return ch;
	}
	Board m_board[M+5];
	ftype Ns[M+5],Psa[M+5][ActionSize+5],s_val[M+5];
	int go[M+5][ActionSize+5],valid[M+5][ActionSize+5],Es[M+5];

	void initNode(int x,const Board &board)
	{
		m_board[x]=board;
		Ns[x]=0;
		memset(Psa[x],0,sizeof(Psa[x]));
		memset(go[x],-1,sizeof(go[x]));
		Es[x]=0;
		if(board.gameState!=GameState::Running)
		{
			Es[x]=1;
			if(board.gameState==GameState::Undecided) s_val[x]=0;
			else 
			{
				int win_id=-1;
				if(board.gameState==GameState::Player0Win) win_id=0;
				else win_id=1;
				if(win_id==board.player_id) s_val[x]=1;
				else s_val[x]=-1;
			}
			return;
		}
		for(int i=0;i<ActionSize;i++) valid[x][i]=board.isValid(i);
		vector<ftype> vec=predictPY(board,valid[x],x==0);
		s_val[x]=vec[0];
		for(int i=0;i<ActionSize;i++) Psa[x][i]=vec[i+1];
	}

	ftype mcts(int &x,const Board &board,int id,int depth=0)
	{
		if(x==-1)
		{
			x=id;
			initNode(x,board);
			Ns[x]++;
			return -s_val[x];
		}

		if(Es[x])
		{
			ftype t=s_val[x]/Ns[x];
			Ns[x]++;
			s_val[x]+=t;
			return -t;
		}

		pair<ftype,int> ch=mp(-100,-1);
		for(int i=0;i<ActionSize;i++)
		{
			if(!valid[x][i]) continue;
			int u=go[x][i];
			ftype qsa=(u==-1?0:s_val[u]/Ns[x]);
			ftype usa=cpuct*Psa[x][i]*sqrt(Ns[x])/(u==-1?1:Ns[u]+1);
			ch=max(ch,mp(qsa+usa,i));
		}
		assert(ch.second!=-1);

		ftype t;
		if(go[x][ch.second]==-1) t=mcts(go[x][ch.second],board.getMove(ch.second),id,depth+1);
		else t=mcts(go[x][ch.second],m_board[go[x][ch.second]],id,depth+1);

		Ns[x]++;
		s_val[x]+=t;
		return -t;
	}

	vector<Operation> solve(int player_id,const GameInfo &info/*,vector<ftype>* actionP=0*/)
	{
		// return vector<Operation>({});
		Board board(player_id,info);
		debug(board.info.round);
		int rt=-1;
		for(int i=0;i<M+1;i++)
		{
			ftype t=mcts(rt,board,i);
			s_val[rt]+=t;
		}

		vector<ftype> p(ActionSize,0);
		if(isTraining)
		{
			for(int i=0;i<ActionSize;i++)
			{
				if(go[rt][i]==-1) p[i]=0;
				else p[i]=(ftype)Ns[go[rt][i]]/M;
			}
			ftype sp=0;
			for(auto &x:p) sp+=x;
			assert(abs(sp-1)<1e-6);
		}
		else
		{
			pair<ftype,int> mx=mp(-1000,-1);
			for(int i=0;i<ActionSize;i++)
			{
				if(go[rt][i]==-1) continue;
				else mx=max(mx,mp(Ns[go[rt][i]],i));
			}
			assert(mx.second!=-1);
			p[mx.second]=1;
		}

		// if(actionP) actionP=new vector<ftype>(p);

		int ch=randomChoose(p);
		vector<Operation> res;
		assert(board.convertToOperation(ch,res));
		return res;
	}
}

void init()
{
	isTraining=true;
	Py_Initialize();
	{
		// PyObject *pModule = PyImport_ImportModule("os");
		// PyObject *pFunc = PyObject_GetAttrString(pModule, "getcwd");
		// PyObject *pArgs = PyTuple_New(0);
		// PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
		// char *cwd;
		// PyArg_Parse(pResult, "s", &cwd);
		PyRun_SimpleString("import sys");
		// cout<<"sys.path.append('~/Saiblo2023/models/')"<<endl;
		PyRun_SimpleString("sys.path.append('/home/monkey/Saiblo2023/models/')");
		// string importDir="sys.path.append('"+(string)(cwd)+"')\n";
		// debug(importDir);
		// PyRun_SimpleString(importDir.c_str());
	}
	PyObject* file=PyImport_ImportModule("model_predict");
	assert(file);
	pFunc=PyObject_GetAttrString(file,"predict");
	assert(pFunc);

	tower_id[TowerType::Basic]=1;
	tower_id[TowerType::Heavy]=2;
	tower_id[TowerType::Quick]=3;
	tower_id[TowerType::Mortar]=4;
	tower_id[TowerType::HeavyPlus]=5;
	tower_id[TowerType::Ice]=6;
	tower_id[TowerType::Cannon]=7;
	tower_id[TowerType::QuickPlus]=8;
	tower_id[TowerType::Double]=9;
	tower_id[TowerType::Sniper]=10;
	tower_id[TowerType::MortarPlus]=11;
	tower_id[TowerType::Pulse]=12;
	tower_id[TowerType::Missile]=13;

	id_tower[1]=TowerType::Basic;
	id_tower[2]=TowerType::Heavy;
	id_tower[3]=TowerType::Quick;
	id_tower[4]=TowerType::Mortar;
	id_tower[5]=TowerType::HeavyPlus;
	id_tower[6]=TowerType::Ice;
	id_tower[7]=TowerType::Cannon;
	id_tower[8]=TowerType::QuickPlus;
	id_tower[9]=TowerType::Double;
	id_tower[10]=TowerType::Sniper;
	id_tower[11]=TowerType::MortarPlus;
	id_tower[12]=TowerType::Pulse;
	id_tower[13]=TowerType::Missile;

	for(int i=0;i<MAP_SIZE;i++)
	{
		for(int j=0;j<MAP_SIZE;j++)
		{
			if(MAP_PROPERTY[i][j]==-1) continue;
			if(MAP_PROPERTY[i][j]==1) continue;
			int y=j;
			int x=(j&1)?18-i:17-i;
			if(MAP_PROPERTY[i][j]==2)
			{
				assert(MAP_PROPERTY[x][y]==3);
				towerPos[0].emplace_back(mp(i,j));
				towerPos[1].emplace_back(mp(x,y));
			}
		}
	}

	cerr<<"----------Initialized----------"<<endl;
}

void play()
{
	Simulator s(GameInfo({seed}));
	int winner=-1;
	for(int i=0;i<512;i++)
	{
		vector<Operation> vec;
		vec=MyAI::solve(0,s.get_info());
		for(auto &op:vec) assert(s.add_operation_of_player(0,op));
		s.apply_operations_of_player(0);
		
		vec=MyAI::solve(1,s.get_info());
		for(auto &op:vec) assert(s.add_operation_of_player(1,op));
		s.apply_operations_of_player(1);

		GameState state=s.next_round();
		if(state!=Running)
		{
			if(state==Player0Win) winner=0;
			else if(state==Player1Win) winner=1;
			else assert(0);
			break;
		}
	}
	cout<<"winner is "<<winner<<endl;
}

int main()
{
	init();
	run_with_ai(MyAI::solve);
	// play();
	Py_Finalize();
	return 0;
}