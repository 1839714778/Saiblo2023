#include "include/simulate.hpp"
#include "include/template.hpp"
#include "include/common.hpp"
#include <bits/stdc++.h>
#include <Python.h>
#define mp make_pair
#define pii pair<int,int>
using namespace std;
const int N=MAP_SIZE;

int tower[N][N];
int ant_max_hp[2][N][N];
int ant_sum_hp[2][N][N];
int tower_id[105];
TowerType id_tower[15];
auto seed=chrono::steady_clock().now().time_since_epoch().count();
mt19937 rng(seed);
vector<pii> towerPos[2];
PyObject* pFunc;
int isTraining;

struct Board
{
	GameInfo info;
	int player_id;
	GameState gameState;
	int convertToOperation(int op,Operation &OP)const{
		if(op<=0) return true;
		op--;
		assert(op<=towerPos[player_id].size()*4);
		int x=towerPos[player_id][op/4].first;
		int y=towerPos[player_id][op/4].second;
		int type=op%4;
		int found_type=0,found_id;
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
				OP=Operation(OperationType::DowngradeTower,found_id,-1);
				if(!info.is_operation_valid(player_id,OP)) return false;
				return true;
			}
		}
		else
		{
			if(found_type>=5) return false;
			else
			{
				if(found_type==0)
				{
					OP=Operation(OperationType::BuildTower,x,y);
					if(!info.is_operation_valid(player_id,OP)) return false;
					return true;
				}
				else
				{
					OP=Operation(OperationType::UpgradeTower,found_id,id_tower[found_type*3+type-2]);
					if(!info.is_operation_valid(player_id,OP)) return false;
					return true;
				}
			}
		}
	}
	int isValid(int x)const
	{
		Operation tmp;
		return convertToOperation(x,tmp);
	}
	Board getMove(int x)const
	{
		Operation op;
		Board board(*this);
		Simulator s(board.info);
		if(convertToOperation(x,op)) 
		{
			s.add_operation_of_player(board.player_id,op);
			s.apply_operations_of_player(board.player_id);
		}
		else
		{
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

		return vec;
	}
	vector<ftype> predictPY(const Board &board,int valid[],int applyDir)
	{
		vector<ftype> seq=paddingBoard(board);
		vector<ftype> vvalid;
		for(int i=0;i<ActionSize;i++) vvalid.emplace_back(valid[i]);
		PyObject *list1=PyList_New(0),*list2=PyList_New(0);
		for(auto &x:seq)
		{
			auto tmp=Py_BuildValue("f",x);
			assert(tmp);
			PyList_Append(list1,tmp);
			Py_DecRef(tmp);
		}
		for(int i=0;i<ActionSize;i++)
		{
			auto tmp=Py_BuildValue("f",valid[i]);
			assert(tmp);
			PyList_Append(list2,tmp);
			Py_DecRef(tmp);
		}
		assert(pFunc);
		PyObject* result=PyObject_CallFunction(pFunc,"(OOi)",list1,list2,applyDir);
		assert(result);
		vector<ftype> res;
		for(int i=0;i<ActionSize;i++)
		{
			PyObject* tmp=PyList_GetItem(result,i);
			assert(tmp);
			res.emplace_back(PyFloat_AsDouble(tmp));
			Py_DecRef(tmp);
		}
		Py_DecRef(list1);
		Py_DecRef(list2);
		Py_DecRef(result);
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
	int go[M+5][ActionSize+5],valid[M+5][ActionSize+5];

	void initNode(int x,const Board &board)
	{
		m_board[x]=board;
		Ns[x]=0;
		memset(Psa[x],0,sizeof(Psa[x]));
		memset(go[x],-1,sizeof(go[x]));
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

	vector<Operation> solve(int player_id,const GameInfo &info)
	{
		Board board;
		board.player_id=player_id;
		board.info=info;
		int rt=-1;
		for(int i=0;i<M+1;i++)
		{
			mcts(rt,board,i);
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

		int ch=randomChoose(p);
		Operation op;
		if(!board.convertToOperation(ch,op))
		{
			assert(ch==0);
			return vector<Operation>({});
		}
		else
		{
			return vector<Operation>({op});
		}
	}
}

void init()
{
	isTraining=true;
	Py_Initialize();
	{
		PyObject *pModule = PyImport_ImportModule("os");
		PyObject *pFunc = PyObject_GetAttrString(pModule, "getcwd");
		PyObject *pArgs = PyTuple_New(0);
		PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
		char *cwd;
		PyArg_Parse(pResult, "s", &cwd);
		PyRun_SimpleString("import sys");
		string importDir="sys.path.append('"+(string)(cwd)+"')\n";
		PyRun_SimpleString(importDir.c_str());
	}
	PyObject* file=PyImport_ImportModule("model_predict");
	PyObject* pFunc=PyObject_GetAttrString(file,"predict");

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
	cerr<<towerPos[0].size()<<' '<<towerPos[1].size()<<endl;
}

int main()
{
	init();
	Py_Finalize();
	return 0;
}