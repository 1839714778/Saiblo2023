#include "include/simulate.hpp"
#include "include/template.hpp"
#include "include/common.hpp"
#include <bits/stdc++.h>
#define mp make_pair
#define pii pair<int,int>
using namespace std;
const int N=MAP_SIZE;
const int ACTION_SIZE=133;

int tower[N][N];
int ant_max_hp[2][N][N];
int ant_sum_hp[2][N][N];
int tower_id[105];
TowerType id_tower[15];
auto seed=chrono::steady_clock().now().time_since_epoch().count();
mt19937 rng(seed);
vector<pii> towerPos[2];

namespace MyAI
{
	using ftype=double;
	vector<ftype> paddingGameInfo(int player_id,const GameInfo &info)
	{
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
	ftype predictWinningRate(int player_id,const GameInfo &info)
	{
		vector<ftype> seq=paddingGameInfo(player_id,info);
		//to do: using python
	}
	vector<ftype> getActionProbability(int player_id,const GameInfo &info)
	{
		vector<ftype> p;
		for(int i=0;i<ACTION_SIZE;i++) p.emplace_back(1);
		return p;
		//to do: using python
	}
	int randomChoose(const vector<ftype> &v)
	{
		assert(v.size());
		ftype tot=0;
		for(auto &x:v) tot+=x;
		ftype x=uniform_real_distribution<ftype>(0,tot)(rng);
		for(int i=0;i<(int)v.size();i++)
		{
			if(x<v[i]) return i;
			x-=v[i];
		}
		return 0;
	}
	int convertOp(int player_id,const GameInfo &info,int op,Operation &OP)
	{
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

	const int ActionSize=133;
	const int M=50;
	GameInfo m_info[M+5];
	ftype Q[M+5],N[M+5],U[M+5],P[M+5][ActionSize+5],go[M+5][ActionSize+5];

	vector<Operation> solve(int player_id,GameInfo info)
	{
		vector<ftype> p=getActionProbability(player_id,info);
		Operation op;
		for(int i=1;i<(int)p.size();i++)
		{
			if(!convertOp(player_id,info,i,op)) p[i]=0;
		}
		int ch=randomChoose(p);
		vector<Operation> res;
		if(ch==0) return res;
		convertOp(player_id,info,ch,op);
		res.emplace_back(op);
		return res;
	}
}

void init()
{
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
	cerr<<sizeof(GameInfo)<<endl;
	init();
	return 0;
}